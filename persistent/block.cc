#include <cstring>
#include <limits>
#include <iterator>
#include <array>
#include <ranges>

#include "crc32c/crc32c.h"

#include "toolpex/assert.h"

#include "frenzykv/persistent/block.h"
#include "frenzykv/util/comp.h"
#include "frenzykv/util/serialize_helper.h"
#include "frenzykv/util/compressor.h"

namespace frenzykv
{

namespace r = ::std::ranges;
namespace rv = r::views;

using ppl_t = uint16_t;
using ril_t = uint32_t;
static constexpr size_t bs_ppl = sizeof(ppl_t);
static constexpr size_t bs_ril = sizeof(ril_t);

static ril_t read_ril(const ::std::byte* cur)
{
    return toolpex::decode_big_endian_from<ril_t>({ cur, bs_ril });
}

template<::std::size_t Len>
static bool filled_with_zero(const ::std::byte* p)
{
    static ::std::array<::std::byte, Len> example{};
    int ret = ::std::memcmp(p, example.data(), Len);
    return ret == 0;
}

block_segment::block_segment(const_bspan block_seg_storage)
    : m_storage{ block_seg_storage }
{
    if ((m_parse_result = parse()) == parse_result_t::error) 
        throw koios::exception{"block_segment: parse fail"};
}

parse_result_t block_segment::parse()
{
    const ::std::byte* current = m_storage.data();
    const ::std::byte* sentinal = current + m_storage.size();

    ppl_t ppl = toolpex::decode_big_endian_from<ppl_t>({ current, bs_ppl });
    if (ppl == 0) return parse_result_t::error;
    current += bs_ppl;
    
    m_prefix = { current, ppl };
    current += ppl;
    
    auto consumed_completely = [sentinal](const ::std::byte* cur){ 
        if (cur >= sentinal) return true;
        return filled_with_zero<bs_ril>(cur);
    };

    while (!consumed_completely(current))
    {
        ril_t r = read_ril(current);
        current += bs_ril;
        m_items.emplace_back(current, r);   
        current += r;
    }

    if (current > sentinal) 
        return parse_result_t::partial;
    else if (filled_with_zero<bs_ril>(current))
    {
        current += 4; // zero-filled RIL OEF
        m_storage = m_storage.subspan(0, current - m_storage.data());
    }

    return parse_result_t::success;
}

bool block_segment::fit_public_prefix(const_bspan user_prefix) const noexcept
{
    if (user_prefix.size() > m_prefix.size()) 
        return false;

    return memcmp_comparator{}(
        user_prefix, m_prefix.subspan(0, user_prefix.size())
    ) == ::std::strong_ordering::equal;
}

bool block_segment::larger_equal_than_this_public_prefix(const_bspan user_prefix) const noexcept
{
    auto cmp_ret = memcmp_comparator{}(
        user_prefix, m_prefix.subspan(0, user_prefix.size())
    );

    return cmp_ret == ::std::strong_ordering::greater
        || cmp_ret == ::std::strong_ordering::equal;
}

bool block_segment::less_than_this_public_prefix(const_bspan user_prefix) const noexcept
{
    auto cmp_ret = memcmp_comparator{}(
        user_prefix, m_prefix.subspan(0, user_prefix.size())
    );
    return cmp_ret == ::std::strong_ordering::less;
}

static ::std::generator<kv_entry> 
entries_from_block_segment_impl(const_bspan uk_from_seg, r::range auto&& items)
{
    // including 2 bytes of user key len
    uk_from_seg = uk_from_seg.subspan(user_key_length_bytes_size);
    for (const auto& item : items)
    {
        ::std::span seq_buffer{ item.data(), sizeof(sequence_number_t) };
        sequence_number_t seq = toolpex::decode_big_endian_from<sequence_number_t>(::std::as_bytes(seq_buffer));
        auto uv_with_len = item.subspan(sizeof(seq));
        uv_with_len = serialized_user_value_from_value_len(uv_with_len);
        co_yield kv_entry{ seq, uk_from_seg, kv_user_value::parse(uv_with_len) };
    }
}

::std::generator<kv_entry> 
entries_from_block_segment(const block_segment& seg)
{
    auto pp = seg.public_prefix();
    const auto& items = seg.items();
    for (auto kv : entries_from_block_segment_impl(pp, items))
    {
        co_yield kv;
    }
}

::std::generator<kv_entry> 
entries_from_block_segment_reverse(const block_segment& seg)
{
    auto pp = seg.public_prefix();
    const auto& items = seg.items();
    auto rview = items | rv::reverse;
    for (auto kv : entries_from_block_segment_impl(pp, rview))
    {
        co_yield kv;
    }
}

// ====================================================================

static const ::std::byte* crc32_beg_ptr(const_bspan storage)
{
    btl_t btl = toolpex::decode_big_endian_from<btl_t>(storage);
    return storage.data() + btl - sizeof(crc32_t);
}

static const ::std::byte* wc_beg_ptr(const_bspan storage)
{
    return crc32_beg_ptr(storage) - 1;
}

wc_t wc_value(const_bspan storage)
{
    return toolpex::decode_big_endian_from<wc_t>({ wc_beg_ptr(storage), sizeof(wc_t) });
}

::std::string block_decompress(
    const_bspan storage, 
    ::std::shared_ptr<compressor_policy> compressor)
{
    toolpex_assert(compressor != nullptr);
    toolpex_assert(wc_value(storage) == 1);
    auto compressed_part = undecompressed_block_content(storage);
    ::std::string result(sizeof(btl_t), 0);
    ::std::error_code ec = compressor->decompress_append_to(compressed_part, result);
    if (ec) throw koios::exception(ec);
    
    result.resize(result.size() + sizeof(wc_t) + sizeof(crc32_t), 0);
    // Re-calculate BTL
    btl_t newbtl = static_cast<btl_t>(result.size());
    ::std::span btl_buffer{ result.data(), sizeof(btl_t) };
    toolpex::encode_big_endian_to(newbtl, btl_buffer);

    return result;
}

bool block_decompress_to(
    const_bspan storage, 
    bspan& dst, 
    ::std::shared_ptr<compressor_policy> compressor)
{
    toolpex_assert(compressor);
    bspan for_block_content = dst.subspan(sizeof(btl_t));
    const bool result = !compressor->decompress(undecompressed_block_content(storage), for_block_content);
    const size_t decompressed_bc_sz = for_block_content.size();
    const size_t new_btl = sizeof(btl_t) + decompressed_bc_sz + sizeof(wc_t) + sizeof(crc32_t);
    toolpex_assert(new_btl <= ::std::numeric_limits<btl_t>::max());
    btl_t new_btl_value = static_cast<btl_t>(new_btl);
    toolpex::encode_big_endian_to(new_btl_value, dst.subspan(0, sizeof(btl_t)));
    dst = dst.subspan(0, new_btl);

    return result;
}

size_t approx_block_decompress_size(
    const_bspan storage, 
    ::std::shared_ptr<compressor_policy> compressor)
{
    toolpex_assert(compressor);
    return compressor->decompressed_minimum_size(
               undecompressed_block_content(storage)) 
        + sizeof(btl_t) + sizeof(crc32_t) + sizeof(wc_t);
}

static btl_t btl_value(const_bspan storage)
{
    btl_t result = toolpex::decode_big_endian_from<btl_t>(storage);
    toolpex_assert(result != 0);
    return result;
}

static const ::std::byte* block_content_beg_ptr(const_bspan storage)
{
    return storage.data() + sizeof(btl_t);
}

const_bspan undecompressed_block_content(const_bspan storage)
{
    return { block_content_beg_ptr(storage), wc_beg_ptr(storage) };
}

crc32_t embeded_crc32_value(const_bspan storage)
{
    const ::std::byte* crc32beg = crc32_beg_ptr(storage);
    return toolpex::decode_big_endian_from<crc32_t>({ crc32beg, sizeof(crc32_t) });
}

static crc32_t calculate_block_content_crc32(const_bspan bc)
{
    return crc32c::Crc32c(reinterpret_cast<const char*>(bc.data()), bc.size());
}

bool block_integrity_check(const_bspan storage)
{
    auto bc = undecompressed_block_content(storage);
    crc32_t crc32 = calculate_block_content_crc32(bc);
    crc32_t ecrc32 = embeded_crc32_value(storage);
    return crc32 == ecrc32;
}

bool block_content_was_comprssed(const_bspan storage)
{
    wc_t wc = toolpex::decode_big_endian_from<wc_t>({ wc_beg_ptr(storage), sizeof(wc_t) });
    toolpex_assert(wc == 1 || wc == 0);
    return wc == 1;
}

// UnCompressed data only
static const ::std::byte* nsbs_beg_ptr(const_bspan s)
{
    btl_t btl = btl_value(s);
    return s.data() + btl - sizeof(nsbs_t) - sizeof(wc_t) - sizeof(crc32_t);
}

// UnCompressed data only
static nsbs_t nsbs_value(const_bspan s)
{
    return toolpex::decode_big_endian_from<nsbs_t>({ nsbs_beg_ptr(s), sizeof(nsbs_t) });
}

// UnCompressed data only
static sbso_t sbso_value(const ::std::byte* sbso_ptr)
{
    return toolpex::decode_big_endian_from<sbso_t>({ sbso_ptr, sizeof(sbso_t) });
}

// UnCompressed data only
static const ::std::byte* meta_data_beg_ptr(const_bspan s)
{
    nsbs_t nsbs = nsbs_value(s);
    return nsbs_beg_ptr(s) - (nsbs * sizeof(sbso_t));
}

block::block(const_bspan block_storage)
    : m_storage{ block_storage }
{
    if ((m_parse_result = parse_meta_data()) == parse_result_t::error)
        throw koios::exception{"block_segment: parse fail"};
    auto segs = segments_in_single_interval();
    m_first_seg_public_prefix = (*begin(segs)).public_prefix();
}

// UnCompressed data only
parse_result_t block::parse_meta_data()
{
    const ::std::byte* sbso_cur = meta_data_beg_ptr(m_storage);
    const ::std::byte* sbso_sentinal = nsbs_beg_ptr(m_storage);
    const auto nsbs = nsbs_value(m_storage);
    if (nsbs == 0) return parse_result_t::error;
    while (sbso_cur < sbso_sentinal)
    {
        m_special_segs.push_back(m_storage.data() + sbso_value(sbso_cur));
        sbso_cur += sizeof(sbso_t);
    }
    toolpex_assert(m_special_segs.size() == nsbs);

    return parse_result_t::success;
}

::std::generator<block_segment> block::
segments_in_single_interval(const ::std::byte* from, const ::std::byte* sentinal) const
{
    const ::std::byte* current = from;
    while (current < sentinal)
    {
        block_segment seg{ { current, static_cast<size_t>(sentinal - current) } };
        if (const auto pr = seg.parse_result(); 
            pr != parse_result_t::success)
        {
            break;
        }
        auto seg_storage = seg.storage();
        current += seg_storage.size();

        // We could not insert the newly parsed seg position in 
        // m_storage to m_special_segs.
        //
        // Like a skip_list, elements of m_special_segs should keep 
        // their distance far enough to ensure the searching performance.
        co_yield ::std::move(seg);
    }
}

::std::generator<block_segment> block::segments_in_single_interval() const
{
    const auto* potential_end = meta_data_beg_ptr(m_storage);
    return segments_in_single_interval(m_special_segs[0], m_special_segs.size() > 1 ? m_special_segs[1]: potential_end);
}

::std::generator<block_segment> block::segments() const
{
    const ::std::byte* current = m_special_segs[0];
    const ::std::byte* sentinal = meta_data_beg_ptr(m_storage);
    while (current < sentinal)
    {
        block_segment seg{ { current, static_cast<size_t>(sentinal - current) } };
        toolpex_assert(seg.parse_result() == parse_result_t::success);
        current += seg.storage().size();
        co_yield ::std::move(seg);
    }
}

::std::optional<block_segment> block::get(const_bspan user_prefix) const
{
    if (!larger_equal_than_this_first_segment_public_prefix(user_prefix))
        return {};

    auto last_segp = m_special_segs.front();
    for (auto [seg1p, seg2p] : m_special_segs | rv::adjacent<2>)
    {
        block_segment seg1{ { seg1p, m_storage.data() + m_storage.size() } };
        block_segment seg2{ { seg2p, m_storage.data() + m_storage.size() } };

        // Shot!
        if (seg1.larger_equal_than_this_public_prefix(user_prefix) 
            && seg2.less_than_this_public_prefix(user_prefix))
        {
            for (auto s : segments_in_single_interval(seg1p, seg2p))
            {
                if (s.fit_public_prefix(user_prefix))
                    return s;
            }
        }
        last_segp = seg2p;
    }

    block_segment last_seg{ { last_segp, m_storage.data() + m_storage.size() } };
    if (last_seg.larger_equal_than_this_public_prefix(user_prefix))
    {
        for (auto s : segments_in_single_interval(last_segp, meta_data_beg_ptr(m_storage)))
        {
            if (s.fit_public_prefix(user_prefix))
                return s;
        }
    }

    return {};
}

bool block::larger_equal_than_this_first_segment_public_prefix(const_bspan cb) const noexcept
{
    auto fspp = first_segment_public_prefix();
    auto comp_ret = memcmp_comparator{}(cb, fspp);
    return comp_ret == ::std::strong_ordering::equal
        || comp_ret == ::std::strong_ordering::greater;
}

bool block::less_than_this_first_segment_public_prefix(const_bspan user_prefix) const noexcept
{
    auto fspp = first_segment_public_prefix();
    auto comp_ret = memcmp_comparator{}(user_prefix, fspp);
    return comp_ret == ::std::strong_ordering::less;
}

block_segment_builder::
block_segment_builder(::std::string& dst, ::std::string_view public_prefix) noexcept
    : m_storage{ dst }, m_public_prefix{ public_prefix }
{
    // Serialize PPL and PP
    ppl_t ppl = static_cast<ppl_t>(public_prefix.size());
    toolpex::append_encode_big_endian_to(ppl, m_storage);
    m_storage.append(public_prefix);
}

bool block_segment_builder::add(const kv_entry& kv)
{
    return add(kv.key(), kv.value());
}   

bool block_segment_builder::add(const sequenced_key& key, const kv_user_value& value)
{
    auto key_rep = key.serialize_user_key_as_string();
    if (key_rep != m_public_prefix)
        return false;

    // Serialize RIL and RI
    ril_t ril = (ril_t)(sizeof(sequence_number_t) + value.serialized_bytes_size());
    toolpex::append_encode_big_endian_to(ril, m_storage);
    key.serialize_sequence_number_append_to(m_storage);
    value.serialize_append_to_string(m_storage);

    return true;
}
       
void block_segment_builder::finish()
{
    if (m_finish) [[unlikely]] return;

    // Write the empty RIL
    ::std::array<char, sizeof(ril_t)> buffer{};
    m_storage.append(buffer.data(), buffer.size());
    m_finish = true;
}

block_builder::block_builder(const kvdb_deps& deps, ::std::shared_ptr<compressor_policy> compressor) 
    : m_deps{ &deps }, m_compressor{ ::std::move(compressor) }
{
    // Typical block size is 4K
    m_storage.reserve(4096);
    // Prepare BTL space
    m_storage.resize(sizeof(btl_t), 0);
}

bool block_builder::add(const kv_entry& kv)
{
    return add(kv.key(), kv.value());
}

bool block_builder::add(const sequenced_key& key, const kv_user_value& value)
{
    toolpex_assert(m_finish == false);

    const size_t interval_sz = m_deps->opt()->max_block_segments_number;

    // At the very beginning of the building phase, there's no seg builder, you need to create one.
    if (!m_current_seg_builder)
    {
        ++m_seg_count;
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, key.serialize_user_key_as_string());
        m_sbsos.push_back(sizeof(btl_t));
    }

    // Segment end, need a new segment.
    if (!m_current_seg_builder->add(key, value))
    {
        // This call will write 4 zero-filled bytes (RIL) to the end of `m_storage`, 
        // indicates termination of the current block segment.
        m_current_seg_builder->finish();

        // You have to make sure that the following key are strictly larger the the last one.
        // Only in the manner, the public prefix compression could give a best performance.
        [[maybe_unused]] auto last_prefix = m_current_seg_builder->public_prefix();
        [[maybe_unused]] auto user_key_rep = key.serialize_user_key_as_string();
        toolpex_assert(memcmp_comparator{}(user_key_rep, last_prefix) == ::std::strong_ordering::greater);
        
        ++m_seg_count;
        if ((m_seg_count + 1) % interval_sz == 0)
        {
            m_sbsos.push_back(static_cast<sbso_t>(m_storage.size()));

            // Every block segment should has a terminator
            // which compitable to RIL(See specification at the top of this file)
            // filled with zero. Means there're no more one.
            toolpex_assert(m_storage[m_storage.size() - 1] == 0);
            toolpex_assert(m_storage[m_storage.size() - 2] == 0);
            toolpex_assert(m_storage[m_storage.size() - 3] == 0);
            toolpex_assert(m_storage[m_storage.size() - 4] == 0);
        }

        // The block_segment_builder won't allocate any space, just simply append new stuff to `m_storage`
        // so there won't be any problem invovlved with the memory management.
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, key.serialize_user_key_as_string());

        [[maybe_unused]] bool add_result = m_current_seg_builder->add(key, value);
        toolpex_assert(add_result);
    }
    // Do not add something like 
    // `m_current_seg_builder->finish()` here
    // cause this function will be called in a loop, to migrate those KVs into sstable
    // this function only covers a very tiny part of that process.
    // No need current seg finish here.

    return true;
}

static ::std::span<char> block_content(::std::string& storage)
{
    return { storage.data() + sizeof(btl_t), storage.size() - sizeof(btl_t) };
}

::std::string block_builder::finish()
{
    toolpex_assert(m_finish == false);
    m_finish = true;
    toolpex_assert(!empty());
    toolpex_assert(m_seg_count);

    // Finish the last block segment builder.
    toolpex_assert(m_current_seg_builder);
    m_current_seg_builder->finish();

    // If the last sbso point to exactly the end of data area, then delete it.
    if (m_sbsos.back() == m_storage.size())
    {
        m_sbsos.erase(::std::prev(m_sbsos.end()));
    }

    // Serialize Meta data area.
    for (sbso_t sbso : m_sbsos)
    {
        toolpex::append_encode_big_endian_to(sbso, m_storage);
    }
    toolpex::append_encode_big_endian_to(static_cast<nsbs_t>(m_sbsos.size()), m_storage);

    auto b_content = block_content(m_storage);

    // Compression
    if (m_compressor && m_deps->opt()->need_compress)
    {
        auto opt_p = m_deps->opt();

        // Prepare memory for BTL
        ::std::string new_storage(sizeof(btl_t), 0);

        // Compress
        auto ec = m_compressor->compress_append_to(
            ::std::as_bytes(b_content), 
            new_storage
        );
        if (ec) throw koios::exception(ec);
        m_compressed = true;

        // Update b_content to the compressed version to calculate CRC32
        b_content = { 
            new_storage.data() + sizeof(btl_t), 
            new_storage.size() - sizeof(btl_t) 
        };

        m_storage = ::std::move(new_storage);
        // WC Only 1 byte
        wc_t wc{ 1 };
        toolpex::append_encode_big_endian_to(wc, m_storage);
    }
    else
    {
        // WC Only 1 byte
        wc_t wc{ 0 };
        toolpex::append_encode_big_endian_to(wc, m_storage);
    }

    // Calculate and append CRC value
    const crc32_t crc = calculate_block_content_crc32(::std::as_bytes(b_content));
    toolpex::append_encode_big_endian_to(crc, m_storage);

    // Serialize BTL
    toolpex_assert(m_storage.size() <= ::std::numeric_limits<btl_t>::max());

    btl_t btl = static_cast<btl_t>(m_storage.size());
    toolpex::encode_big_endian_to(btl, { m_storage.data(), sizeof(btl) });

    return ::std::move(m_storage);
}

::std::generator<kv_entry> block::entries() const
{
    for (block_segment seg : segments())
    {
        for (auto entry : entries_from_block_segment(seg))
        {
            co_yield ::std::move(entry);
        }
    }
}

} // namespace frenzykv
