#include "crc32c/crc32c.h"
#include "frenzykv/persistent/block.h"
#include "frenzykv/util/comp.h"
#include "frenzykv/util/serialize_helper.h"
#include "frenzykv/util/compressor.h"
#include <cstring>
#include <limits>
#include <cassert>
#include <iterator>
#include <array>

namespace frenzykv
{

namespace rv = ::std::ranges::views;

/*  block segment format
 *
 *  PP:         string      public prefix
 *  PPL:    2B  uint16_t    public prefix length
 *  RIL:    4B  uint32_t    rest item length
 *  RI:         string      rest item
 *
 *  | PPL |      PP      | RIL |      RI       | RIL |     RI     | ... | 4B Zero EOF (RIL) |
 *
 *
 */

using ppl_t = uint16_t;
using ril_t = uint32_t;
static constexpr size_t bs_ppl = sizeof(ppl_t);
static constexpr size_t bs_ril = sizeof(ril_t);

static ril_t read_ril(const ::std::byte* cur)
{
    ril_t result{};
    ::std::memcpy(&result, cur, bs_ril);
    return result;
}

template<::std::size_t Len>
static bool filled_with_zero(const ::std::byte* p)
{
    static ::std::array<::std::byte, Len> example{};
    int ret = ::std::memcmp(p, example.data(), Len);
    return ret == 0;
}

parse_result_t block_segment::parse()
{
    const ::std::byte* current = m_storage.data();
    const ::std::byte* sentinal = current + m_storage.size();

    ppl_t ppl{};
    ::std::memcpy(&ppl, current, bs_ppl);
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

// ====================================================================

static const ::std::byte* crc32_beg_ptr(const_bspan storage)
{
    btl_t btl{};
    ::std::memcpy(&btl, storage.data(), sizeof(btl_t));
    return storage.data() + btl - sizeof(crc32_t);
}

static const ::std::byte* wc_beg_ptr(const_bspan storage)
{
    return crc32_beg_ptr(storage) - 1;
}

wc_t wc_value(const_bspan storage)
{
    wc_t result{};
    const ::std::byte* wcp = wc_beg_ptr(storage);
    ::std::memcpy(&result, wcp, sizeof(wc_t));
    return result;
}

::std::string block_decompress(
    const_bspan storage, 
    ::std::shared_ptr<compressor_policy> compressor)
{
    assert(compressor != nullptr);
    assert(wc_value(storage) == 1);
    auto compressed_part = undecompressed_block_content(storage);
    ::std::string result(sizeof(btl_t), 0);
    ::std::error_code ec = compressor->decompress_append_to(compressed_part, result);
    if (ec) throw koios::exception(ec);
    
    result.resize(result.size() + sizeof(wc_t) + sizeof(crc32_t), 0);
    // Re-calculate BTL
    btl_t newbtl = static_cast<btl_t>(result.size());
    ::std::memcpy(result.data(), &newbtl, sizeof(btl_t));

    return result;
}

static btl_t btl_value(const_bspan storage)
{
    btl_t result{};
    ::std::memcpy(&result, storage.data(), sizeof(btl_t));
    assert(result != 0);
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
    crc32_t result{};
    ::std::memcpy(&result, crc32beg, sizeof(crc32_t));
    return result;
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
    wc_t wc{};
    ::std::memcpy(&wc, wc_beg_ptr(storage), sizeof(wc));
    assert(wc == 1 || wc == 0);
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
    nsbs_t result{};
    const auto* p = nsbs_beg_ptr(s);
    ::std::memcpy(&result, p, sizeof(nsbs_t));

    return result;
}

// UnCompressed data only
static sbso_t sbso_value(const ::std::byte* sbso_ptr)
{
    sbso_t result{};
    ::std::memcpy(&result, sbso_ptr, sizeof(sbso_t));
    return result;
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
    m_first_seg_public_prefix = ::std::begin(segs)->public_prefix();
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
    assert(m_special_segs.size() == nsbs);

    return parse_result_t::success;
}

koios::generator<block_segment> block::
segments_in_single_interval(::std::vector<const ::std::byte*>::const_iterator insert_iter)
{
    const ::std::byte* from = *insert_iter;
    const ::std::byte* current = from;
    
    auto parsing_end_iter = ::std::next(insert_iter);

    const ::std::byte* const sentinal = 
        (parsing_end_iter == m_special_segs.end()) ? meta_data_beg_ptr(m_storage) : *parsing_end_iter;

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

::std::optional<block_segment> block::get(const_bspan user_prefix) const
{
    if (!larger_equal_than_this_first_segment_public_prefix(user_prefix))
        return {};

    auto sp_seg = m_special_segs
        | rv::transform([this](auto&& ptr){
              return block_segment{ { ptr, m_storage.data() + m_storage.size() } };
          });

    block_segment last_seg = *begin(sp_seg);
    for (auto curwin : sp_seg | rv::slide(2))
    {
        block_segment seg1 = curwin[0];
        block_segment seg2 = curwin[1];

        // Shot!
        if (seg1.larger_equal_than_this_public_prefix(user_prefix) 
            && seg2.less_than_this_public_prefix(user_prefix))
        {
            return seg1;
        }
        last_seg = ::std::move(seg2);
    }

    if (last_seg.larger_equal_than_this_public_prefix(user_prefix))
        return last_seg;

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
    auto comp_ret = memcmp_comparator{}(cb, fspp);
    return comp_ret == ::std::strong_ordering::less;
}

block_segment_builder::
block_segment_builder(::std::string& dst, ::std::string_view public_prefix) noexcept
    : m_storage{ dst }, m_public_prefix{ public_prefix }
{
    // Serialize PPL and PP
    ppl_t ppl = static_cast<ppl_t>(public_prefix.size());
    append_encode_int_to<sizeof(ppl)>(ppl, m_storage);
    m_storage.append(public_prefix);
}

bool block_segment_builder::add(const kv_entry& kv)
{
    return add(kv.key(), kv.value());
}   

bool block_segment_builder::add(const sequenced_key& key, const kv_user_value& value)
{
    if (key.user_key() != m_public_prefix)
        return false;

    // Serialize RIL and RI
    ril_t ril = (ril_t)(sizeof(sequence_number_t) + value.serialized_bytes_size());
    append_encode_int_to<sizeof(ril)>(ril, m_storage);
    key.serialize_sequence_number_append_to(m_storage);
    value.serialize_append_to_string(m_storage);

    return true;
}
       
void block_segment_builder::finish()
{
    if (m_finish) [[unlikely]] return;

    // Write the BTL
    ::std::array<char, sizeof(btl_t)> buffer{};
    m_storage.append(buffer.data(), buffer.size());
    m_finish = true;
}

block_builder::block_builder(const kvdb_deps& deps, ::std::shared_ptr<compressor_policy> compressor) 
    : m_deps{ &deps }, m_compressor{ ::std::move(compressor) }
{
    // Prepare BTL space
    m_storage.resize(sizeof(btl_t), 0);
}

bool block_builder::add(const kv_entry& kv)
{
    return add(kv.key(), kv.value());
}

bool block_builder::add(const sequenced_key& key, const kv_user_value& value)
{
    assert(m_finish == false);

    const size_t interval_sz = m_deps->opt()->max_block_segments_number;

    // At the very beginning of the building phase, there's no seg builder, you need to create one.
    if (!m_current_seg_builder)
    {
        ++m_seg_count;
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, key.user_key());
        m_sbsos.push_back(sizeof(btl_t));
    }

    // Segment end, need a new segment.
    if (!m_current_seg_builder->add(key, value))
    {
        // This call will write 4 zero-filled bytes to the end of `m_storage`, 
        // indicates termination of the current block segment.
        m_current_seg_builder->finish();

        // You have to make sure that the following key are strictly larger the the last one.
        // Only in the manner, the public prefix compression could give a best performance.
        [[maybe_unused]] auto last_prefix = m_current_seg_builder->public_prefix();
        [[maybe_unused]] auto user_key = key.user_key();
        assert(memcmp_comparator{}(user_key, last_prefix) == ::std::strong_ordering::greater);
        
        ++m_seg_count;
        if ((m_seg_count + 1) % interval_sz == 0)
        {
            m_sbsos.push_back(static_cast<sbso_t>(m_storage.size()));

            // Every block segment should has a terminator
            // which compitable to RIL(See specification at the top of this file)
            // filled with zero. Means there're no more one.
            assert(m_storage[m_storage.size() - 1] == 0);
            assert(m_storage[m_storage.size() - 2] == 0);
            assert(m_storage[m_storage.size() - 3] == 0);
            assert(m_storage[m_storage.size() - 4] == 0);
        }

        // The block_segment_builder won't allocate any space, just simply append new stuff to `m_storage`
        // so there won't be any problem invovlved with the memory management.
        m_current_seg_builder = ::std::make_unique<block_segment_builder>(m_storage, key.user_key());

        [[maybe_unused]] bool add_result = m_current_seg_builder->add(key, value);
        assert(add_result);
    }

    return true;
}

static ::std::span<char> block_content(::std::string& storage)
{
    return { storage.data() + sizeof(btl_t), storage.size() - sizeof(btl_t) };
}

::std::string block_builder::finish()
{
    assert(m_finish == false);
    m_finish = true;

    // If the last sbso point to exactly the end of data area, then delete it.
    if (m_sbsos.back() == m_storage.size())
    {
        m_sbsos.erase(::std::prev(m_sbsos.end()));
    }

    // Serialize Meta data area.
    for (sbso_t sbso : m_sbsos)
    {
        append_encode_int_to<sizeof(sbso_t)>(sbso, m_storage);
    }
    append_encode_int_to<sizeof(nsbs_t)>(static_cast<nsbs_t>(m_sbsos.size()), m_storage);

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
        append_encode_int_to<sizeof(uint8_t)>(wc, m_storage);
    }
    else
    {
        // WC Only 1 byte
        wc_t wc{ 0 };
        append_encode_int_to<sizeof(uint8_t)>(wc, m_storage);
    }

    // Calculate and append CRC value
    const crc32_t crc = calculate_block_content_crc32(::std::as_bytes(b_content));
    append_encode_int_to<sizeof(crc32_t)>(crc, m_storage);

    // Serialize BTL
    assert(m_storage.size() <= ::std::numeric_limits<btl_t>::max());

    btl_t btl = static_cast<btl_t>(m_storage.size());
    encode_int_to<sizeof(btl_t)>(btl, ::std::as_writable_bytes(::std::span{ m_storage.data(), sizeof(btl) }));

    return ::std::move(m_storage);
}

} // namespace frenzykv
