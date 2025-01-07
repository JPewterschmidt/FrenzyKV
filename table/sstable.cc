// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <string>
#include <iterator>
#include <ranges>
#include <list>

#include "toolpex/exceptions.h"
#include "toolpex/assert.h"

#include "frenzykv/table/sstable.h"
#include "frenzykv/table/sstable_builder.h"
#include "frenzykv/util/comp.h"
#include "frenzykv/util/serialize_helper.h"

namespace frenzykv
{

namespace rv = ::std::ranges::views;

sstable::sstable(const kvdb_deps& deps, 
                 filter_policy* filter, 
                 random_readable* file)
    : m_deps{ &deps },
      m_file{ file }, 
      m_filter{ filter },
      m_compressor{ get_compressor(*m_deps->opt(), m_deps->opt()->compressor_name) }, 
      m_hash_value{ ::std::hash<::std::string_view>{}(m_file->filename()) }
{
    toolpex_assert(m_compressor);
    toolpex_assert(m_filter);
}

sstable::sstable(const kvdb_deps& deps, 
                 filter_policy* filter, 
                 ::std::unique_ptr<random_readable> file)
    : m_self_managed_file{ ::std::move(file) }, 
      m_deps{ &deps }, 
      m_file{ m_self_managed_file.get() }, 
      m_filter{ filter },
      m_compressor{ get_compressor(*m_deps->opt(), m_deps->opt()->compressor_name) },
      m_hash_value{ ::std::hash<::std::string_view>{}(m_file->filename()) }
{
    toolpex_assert(m_compressor);
    toolpex_assert(m_filter);
}

bool sstable::empty() const noexcept
{
    return m_file == nullptr;
}

koios::task<bool> sstable::parse_meta_data()
{ 
    if (m_meta_data_parsed.load()) co_return true;

    if (m_file == nullptr)
    {
        m_meta_data_parsed = true;
        co_return true;
    }
    const uintmax_t filesz = m_file->file_size(); 

    const size_t footer_sz = sizeof(mbo_t) + sizeof(mgn_t);
    ::std::string buffer(footer_sz, 0);
    co_await m_file->read({ buffer.data(), buffer.size() }, filesz - footer_sz);
    const ::std::byte* buffer_beg = reinterpret_cast<::std::byte*>(buffer.data());
    mbo_t mbo = toolpex::decode_big_endian_from<mbo_t>({ buffer_beg, sizeof(mbo_t) });
    mgn_t magic_num = toolpex::decode_big_endian_from<mgn_t>({ buffer_beg + sizeof(mbo_t), sizeof(magic_num) });

    // file integrity check
    if (magic_number_value() != magic_num)
    {
        co_return false;
    }

    // For meta Block
    buffer = ::std::string(filesz - mbo - sizeof(mbo_t) - sizeof(mgn_t), 0);
    auto sp = ::std::as_bytes(::std::span{buffer});
    co_await m_file->read({ buffer.data(), buffer.size() }, mbo);

    toolpex_assert(block_integrity_check(sp));
    block meta_block{ sp };
    toolpex_assert(meta_block.special_segments_count() == 1);
    for (block_segment seg : meta_block.segments_in_single_interval())
    {
        sequenced_key filter_key{ 0, "bloom_filter" };
        sequenced_key last_uk_key{ 0, "last_uk" };
        sequenced_key first_uk_key{ 0, "first_uk" };
        auto filter_key_rep = filter_key.serialize_user_key_as_string();
        auto last_uk_rep = last_uk_key.serialize_user_key_as_string();
        auto first_uk_rep = first_uk_key.serialize_user_key_as_string();
        if (as_string_view(seg.public_prefix()) == filter_key_rep)
        {
            auto fake_user_value_sp_with_seq = seg.items().front();
            auto uv = kv_user_value::parse(fake_user_value_sp_with_seq.subspan(sizeof(sequence_number_t)));
            m_filter_rep = uv.value();
        }
        else if (as_string_view(seg.public_prefix()) == last_uk_rep)
        {
            auto fake_user_value_sp_with_seq = seg.items().front();
            auto last_uk = kv_user_value::parse(fake_user_value_sp_with_seq.subspan(sizeof(sequence_number_t)));
            m_last_uk = last_uk.value();
        }
        else if (as_string_view(seg.public_prefix()) == first_uk_rep)
        {
            auto fake_user_value_sp_with_seq = seg.items().front();
            auto first_uk = kv_user_value::parse(fake_user_value_sp_with_seq.subspan(sizeof(sequence_number_t)));
            m_first_uk = first_uk.value();
        }
    }

    toolpex_assert(m_filter_rep.size() != 0);
    toolpex_assert(m_last_uk.size() != 0);
    toolpex_assert(m_first_uk.size() != 0);
    if (!co_await generate_block_offsets_impl(mbo)) 
        co_return false;

    m_meta_data_parsed = true;
    co_return true;
}

koios::task<btl_t> sstable::btl_value_impl(uintmax_t offset)
{
    ::std::array<::std::byte, sizeof(btl_t)> buffer{};
    ::std::memset(buffer.data(), 0, buffer.size());

    if (!co_await m_file->read(buffer, offset))
        co_return 0;

    co_return toolpex::decode_big_endian_from<btl_t>({ buffer.data(), sizeof(btl_t) });
}

koios::task<bool> sstable::generate_block_offsets_impl(mbo_t mbo)
{
    btl_t current_btl{};
    uintmax_t offset{};
    while (offset < mbo)
    {
        current_btl = co_await btl_value_impl(offset);
        if (current_btl == 0)
            co_return false;

        m_block_offsets.emplace_back(offset, current_btl);
        offset += current_btl;
    }
    co_return true;
}

koios::task<::std::optional<block>> 
sstable::get_block(uintmax_t offset, btl_t btl) const
{
    ::std::optional<block> result{};

    buffer<> buff{btl + 10}; // extra bytes to avoid unknow reason buffer overflow.
    
    size_t readed = co_await m_file->read(buff.writable_span().subspan(0, btl), offset);
    toolpex_assert(readed == btl);
    buff.commit(readed);

    const_bspan bs = buff.valid_span();

    if (!block_integrity_check(bs))
        co_return result;

    if (block_content_was_comprssed(bs))
    {
        size_t minimum_buffer_sz = approx_block_decompress_size(bs, m_compressor);
        buffer<> buf{ minimum_buffer_sz };
        auto w = buf.writable_span();
        if (!block_decompress_to(bs, w, m_compressor))
            co_return result;
        buf.commit(w.size());
        
        bs = w;
        result.emplace(bs, ::std::move(buf));
        co_return result;
    }
    
    result.emplace(bs, ::std::move(buff));
    co_return result;
}

koios::task<::std::optional<::std::pair<block_segment, block>>> 
sstable::
get_segment(const sequenced_key& user_key_ignore_seq) const
{
    auto user_key_rep = user_key_ignore_seq.serialize_user_key_as_string();
    auto user_key_rep_b = ::std::as_bytes(::std::span{ user_key_rep });
    if (!m_filter->may_match(user_key_rep_b, m_filter_rep))
        co_return {};

    auto blk_aws = m_block_offsets
        | rv::transform([this](auto&& pair){ 
              return this->get_block(pair.first, pair.second);
          });

    auto last_block_opt = co_await *begin(blk_aws);
    for (auto [blk0aw, blk1aw] : blk_aws | rv::adjacent<2>)
    {
        auto blk0_opt = co_await blk0aw;
        auto blk1_opt = co_await blk1aw;

        toolpex_assert(blk0_opt.has_value());
        toolpex_assert(blk1_opt.has_value());

        // Shot!
        if (blk0_opt->larger_equal_than_this_first_segment_public_prefix(user_key_rep_b)
           && blk1_opt->less_than_this_first_segment_public_prefix(user_key_rep_b))
        {
            auto seg_opt = blk0_opt->get(user_key_rep_b);
            if (seg_opt)
            {
                co_return ::std::pair{ ::std::move(*seg_opt), ::std::move(*blk0_opt) };
            }
            else co_return {};
        }
        
        last_block_opt = ::std::move(blk1_opt);
    }
    if (last_block_opt->larger_equal_than_this_first_segment_public_prefix(user_key_rep_b))
    {
        auto seg_opt = last_block_opt->get(user_key_rep_b);
        if (seg_opt) 
        {
            co_return ::std::pair{ ::std::move(*seg_opt), ::std::move(*last_block_opt) };
        }
    }

    co_return {};
}

koios::task<::std::optional<kv_entry>>
sstable::
get_kv_entry(const sequenced_key& user_key) const
{
    auto seg_opt = co_await get_segment(user_key);
    if (!seg_opt) co_return {};
    const auto& seg = seg_opt->first;
    
    for (kv_entry entry : entries_from_block_segment_reverse(seg))
    {
        if (entry.key().sequence_number() <= user_key.sequence_number()) 
            co_return entry;
    }

    co_return {};
}

sequenced_key sstable::last_user_key_without_seq() const noexcept
{
    ::std::string temp = m_last_uk;
    temp.resize(temp.size() + 8);
    auto tempb = ::std::span{ temp };
    sequenced_key result(::std::as_bytes(tempb));
    return result;
}

sequenced_key sstable::first_user_key_without_seq() const noexcept
{
    ::std::string temp = m_first_uk;
    temp.resize(temp.size() + 8);
    auto tempb = ::std::span{ temp };
    sequenced_key result(::std::as_bytes(tempb));
    return result;
}

bool sstable::overlapped(const disk_table& other) const noexcept
{
    return !disjoint(other);
}

bool sstable::disjoint(const disk_table& other) const noexcept
{
    /*
     *        A               B
     *   al|-----|ar    bl|-------|br
     *
     *         B               A     
     *   bl|-------|br    al|-----|ar
     *
     */
    const sequenced_key al = first_user_key_without_seq();
    const sequenced_key ar = last_user_key_without_seq();
    const sequenced_key bl = other.first_user_key_without_seq();
    const sequenced_key br = other.last_user_key_without_seq();

    return (al < bl && ar < bl) || (bl < al && br < al);
}

::std::generator<::std::pair<uintmax_t, btl_t>> 
sstable::
block_offsets() const noexcept
{
    for (auto p : m_block_offsets)
    {
        co_yield p;
    }
}

koios::generator<kv_entry>
get_entries_from_sstable(sstable& table)
{
    if (table.empty()) co_return;
    for (auto blk_off : table.block_offsets())
    {
        auto blk_opt = co_await table.get_block(blk_off);
        toolpex_assert(blk_opt.has_value());
        for (auto kv : blk_opt->entries())
        {
            co_yield ::std::move(kv);
        }
    }
}

::std::string_view sstable::filename() const noexcept
{
    if (empty())
    {
        return {};
    }

    return m_file->filename();
}

bool sstable::operator==(const disk_table& other) const noexcept
{
    if (empty()) return other.empty();
    return filename() == other.filename();
}

} // namespace frenzykv
