#include <string>

#include "toolpex/exceptions.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"
#include "frenzykv/util/comp.h"

namespace frenzykv
{

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

static ::std::optional<block_segment> 
get_segment_from_block(const block& b, const_bspan user_key)
{
    // TODO
    return {};
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
    
    for (const auto& [offset, btl] : m_block_offsets)
    {
        auto opt = co_await get_block(offset, btl);
        if (!opt) co_return {};
        auto cur_uk = opt->first_segment_public_prefix();

        // TODO There should be check the range, of course, including the beg, and the last public_prefix of the current block
        if (auto cmp_ret = memcmp_comparator{}(cur_uk, user_key);
            cmp_ret == ::std::strong_ordering::equal
            /*TODO*/)
        {
            co_return get_segment_from_block(opt.value(), user_key);
        }
    }

    co_return {};
}

} // namespace frenzykv
