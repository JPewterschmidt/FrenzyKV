#include <cassert>

#include "frenzykv/persistent/sstable_builder.h"
#include "frenzykv/util/serialize_helper.h"

namespace frenzykv
{

static constexpr mgn_t magic_number = 0x47d6ddc3;

mgn_t magic_number_value() noexcept
{
    return magic_number;
}

sstable_builder::sstable_builder(
        const kvdb_deps& deps, 
        uintmax_t size_limit,
        filter_policy* filter, 
        seq_writable* file)
    : m_deps{ &deps }, 
      m_size_limit{ size_limit },
      m_filter{ filter }, 
      m_block_builder{ *m_deps, get_compressor(*m_deps->opt(), "zstd") },
      m_file{ file }
{
    assert(m_size_limit != 0);
}

koios::task<bool> sstable_builder::add(
    const sequenced_key& key, const kv_user_value& value)
{
    assert(!was_finish());
    assert(m_filter != nullptr);
    if (reach_the_size_limit())
    {
        co_return false;
    }

    if (m_first_uk.empty())
    {
        m_first_uk = key.serialize_user_key_as_string();
    }
    
    // Including the length encoded part at the begging of the key_rep
    auto key_rep = key.serialize_user_key_as_string();
    if (key_rep != m_last_uk)
    {
        m_filter->append_new_filter(key_rep, m_filter_rep);
        m_last_uk = key_rep;
    }

    if (!m_block_builder.add(key, value))
    {
        co_return false;
    }

    // Flush to file
    if (m_block_builder.bytes_size() >= m_deps->opt()->block_size)
    {
        bool ret = co_await flush_current_block();
        if (!ret) co_return false;
        m_size_wrote += m_block_builder.bytes_size();
    }

    co_return true;
}

koios::task<bool> sstable_builder::flush_current_block(bool need_flush)
{
    auto block_storage = m_block_builder.finish();
    m_block_builder = { *m_deps, m_block_builder.compressor() };
    
    ::std::span cb{ block_storage };
    size_t wrote = co_await m_file->append(::std::as_bytes(cb));

    if (need_flush)
    {
        //  Has to make sure the contents has been successfully wrote.
        co_await m_file->flush(); 
    }

    const bool result{ wrote == block_storage.size() };
    if (result) m_bytes_appended_to_file += wrote;

    co_return result;
}

koios::task<bool> sstable_builder::finish()
{
    assert(!was_finish());
    m_finish = true;

    if (!m_block_builder.was_finish())
    {
        co_await flush_current_block(false); // wont flush.
    }
    
    // Build meta block
    block_builder meta_builder{ *m_deps };
    meta_builder.add({0, "last_uk"}, m_last_uk);
    meta_builder.add({0, "first_uk"}, m_first_uk);
    meta_builder.add({0, "bloom_filter"}, m_filter_rep);
    m_block_builder = ::std::move(meta_builder);

    const mbo_t mbo = m_bytes_appended_to_file;
    co_await flush_current_block(false);

    ::std::array<::std::byte, sizeof(mbo) + sizeof(magic_number)> mbo_and_magic_number_buffer{};
    toolpex::encode_big_endian_to(mbo, mbo_and_magic_number_buffer);
    toolpex::encode_big_endian_to(magic_number, ::std::span{mbo_and_magic_number_buffer}.subspan(sizeof(mbo)));

    co_await m_file->append(mbo_and_magic_number_buffer);
    co_await m_file->close();
    co_return true;
}

} // namespace frenzykv
