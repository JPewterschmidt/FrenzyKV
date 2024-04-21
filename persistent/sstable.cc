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
                 ::std::unique_ptr<random_readable> file, 
                 ::std::unique_ptr<filter_policy> filter,
                 ::std::shared_ptr<compressor_policy> compressor)
    : m_deps{ &deps }, 
      m_file{ ::std::move(file) }, 
      m_filter{ ::std::move(filter) },
      m_compressor{ ::std::move(compressor) }
{
    assert(m_compressor);
    assert(m_filter);
}

koios::task<bool> sstable::parse_meta_data()
{
    const uintmax_t filesz = m_file->file_size();
    ::std::string buffer(sizeof(mbo_t) + sizeof(mgn_t), 0);
    
    co_await m_file->read({ buffer.data(), buffer.size() }, filesz);
    mbo_t mbo{};
    mgn_t magic_num{};
    ::std::memcpy(&mbo, buffer.data(), sizeof(mbo));
    ::std::memcpy(&magic_num, buffer.data(), sizeof(magic_num));

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
            auto fake_user_value_sp = seg.items().front();
            auto uv = kv_user_value::parse(fake_user_value_sp);
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

koios::task<::std::optional<block>> 
sstable::get_block(uintmax_t offset, btl_t btl)
{
    ::std::optional<block> result{};

    m_buffer.clear();
    m_buffer.resize(btl, 0);
    [[maybe_unused]] size_t readed = co_await m_file->read({ m_buffer.data(), m_buffer.size() }, offset);
    assert(readed == btl);

    const_bspan bs{ ::std::as_bytes(::std::span{m_buffer}) };

    if (!block_integrity_check(bs))
        co_return result;

    if (block_content_was_comprssed(bs))
    {
        m_buffer = block_decompress(bs, m_compressor);
        bs = { ::std::as_bytes(::std::span{m_buffer}) };
    }
    
    result.emplace(bs);
    co_return result;
}

koios::task<::std::optional<block_segment>> 
sstable::get(const_bspan user_key)
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
        if (blk0_opt->larger_equal_than_this_first_segment_public_prefix(user_key)
           && blk1_opt->less_than_this_first_segment_public_prefix(user_key))
        {
            co_return blk0_opt->get(user_key);
        }
        
        last_block_opt = ::std::move(blk1_opt);
    }
    if (last_block_opt->larger_equal_than_this_first_segment_public_prefix(user_key))
    {
        co_return last_block_opt->get(user_key);
    }

    co_return {};
}

} // namespace frenzykv
