#include <string>
#include <iterator>
#include <ranges>

#include "toolpex/exceptions.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"
#include "frenzykv/util/comp.h"

namespace frenzykv
{

namespace rv = ::std::ranges::views;

sstable::sstable(const kvdb_deps& deps, 
                 ::std::unique_ptr<filter_policy> filter, 
                 ::std::unique_ptr<random_readable> file)
    : m_deps{ &deps }, 
      m_file{ ::std::move(file) }, 
      m_filter{ ::std::move(filter) },
      m_compressor{ get_compressor(*m_deps->opt(), m_deps->opt()->compressor_name) }
{
    assert(m_compressor);
    assert(m_filter);
}

koios::task<bool> sstable::parse_meta_data()
{
    const uintmax_t filesz = m_file->file_size();
    const size_t footer_sz = sizeof(mbo_t) + sizeof(mgn_t);
    ::std::string buffer(footer_sz, 0);
    co_await m_file->read({ buffer.data(), buffer.size() }, filesz - footer_sz);
    mbo_t mbo{};
    mgn_t magic_num{};
    ::std::memcpy(&mbo, buffer.data(), sizeof(mbo));
    ::std::memcpy(&magic_num, buffer.data() + sizeof(mbo), sizeof(magic_num));

    // file integrity check
    if (magic_number_value() != magic_num)
    {
        co_return false;
    }

    // For meta Block
    buffer = ::std::string(filesz - mbo - sizeof(mbo_t) - sizeof(mgn_t), 0);
    co_await m_file->read({ buffer.data(), buffer.size() }, mbo);
    block meta_block{ ::std::as_bytes(::std::span{buffer}) };
    assert(meta_block.special_segments_count() == 1);
    for (block_segment seg : meta_block.segments_in_single_interval())
    {
        if (as_string_view(seg.public_prefix()) == "bloom_filter")
        {
            auto fake_user_value_sp_with_seq = seg.items().front();
            auto uv = kv_user_value::parse(fake_user_value_sp_with_seq.subspan(sizeof(sequence_number_t)));
            m_filter_rep = uv.value();
            break; // Until now, there should be only one segment, only one element in segment.
        }
    }

    if (!co_await generate_block_offsets()) 
        co_return false;

    m_meta_data_parsed = true;
    co_return true;
}

koios::task<btl_t> sstable::btl_value(uintmax_t offset)
{
    static ::std::array<::std::byte, sizeof(btl_t)> buffer{};
    btl_t result{};
    ::std::memset(buffer.data(), 0, buffer.size());

    if (!co_await m_file->read(buffer, offset))
        co_return 0;

    ::std::memcpy(&result, buffer.data(), sizeof(btl_t));
    co_return result;
}

koios::task<bool> sstable::generate_block_offsets()
{
    btl_t current_btl{};
    uintmax_t offset{};
    const uintmax_t filesz = m_file->file_size();
    while (offset < filesz)
    {
        current_btl = co_await btl_value(offset);
        if (current_btl == 0)
            co_return false;

        m_block_offsets.emplace_back(offset, current_btl);
        offset += current_btl;
    }
    co_return true;
}

koios::task<::std::optional<sstable::block_with_storage>> 
sstable::get_block(uintmax_t offset, btl_t btl)
{
    ::std::optional<sstable::block_with_storage> result{};

    m_buffer.clear();
    m_buffer.resize(btl, 0);
    [[maybe_unused]] size_t readed = co_await m_file->read({ m_buffer.data(), m_buffer.size() }, offset);
    assert(readed == btl);

    const_bspan bs{ ::std::as_bytes(::std::span{m_buffer}) };

    if (!block_integrity_check(bs))
        co_return result;

    if (block_content_was_comprssed(bs))
    {
        auto new_storage = block_decompress(bs, m_compressor);
        bs = { ::std::as_bytes(::std::span{new_storage}) };
        result.emplace(block(bs), ::std::move(new_storage));
        co_return result;
    }
    
    result.emplace(block(bs), ::std::string());
    co_return result;
}

koios::task<::std::optional<block_segment>> 
sstable::get_segment(const_bspan user_key)
{
    if (!m_meta_data_parsed)
        co_await parse_meta_data();

    if (!m_filter->may_match(user_key, m_filter_rep))
        co_return {};

    // Re-allocate space from heap, to shrink the memory useage.
    if (++m_get_call_count % 128 == 0)
    {
        m_buffer = {};
    }

    auto blk_aws = m_block_offsets
        | rv::transform([this](auto&& pair){ 
              return this->get_block(pair.first, pair.second);
          });

    auto last_block_opt = co_await *begin(blk_aws);
    for (auto window : blk_aws | rv::slide(2))
    {
        auto blk0_opt = co_await window[0];
        auto blk1_opt = co_await window[1];

        assert(blk0_opt.has_value());
        assert(blk1_opt.has_value());

        // Shot!
        if (blk0_opt->b.larger_equal_than_this_first_segment_public_prefix(user_key)
           && blk1_opt->b.less_than_this_first_segment_public_prefix(user_key))
        {
            if (auto& ss = blk0_opt->s; !ss.empty())
                m_buffer = ::std::move(ss);
            co_return blk0_opt->b.get(user_key);
        }
        
        last_block_opt = ::std::move(blk1_opt);
    }
    if (last_block_opt->b.larger_equal_than_this_first_segment_public_prefix(user_key))
    {
        co_return last_block_opt->b.get(user_key);
    }

    co_return {};
}

koios::task<::std::optional<kv_entry>>
sstable::
get_kv_entry(sequence_number_t seq, const_bspan user_key)
{
    auto seg_opt = co_await get_segment(user_key);
    if (!seg_opt) co_return {};
    const auto& seg = *seg_opt;
    
    for (kv_entry entry : entries_from_block_segment(seg))
    {
        if (entry.key().sequence_number() >= seq) 
            co_return entry;
    }

    co_return {};
}

} // namespace frenzykv
