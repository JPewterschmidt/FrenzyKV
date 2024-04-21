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
        ::std::unique_ptr<filter_policy> filter, 
        ::std::unique_ptr<seq_writable> file)
    : m_deps{ &deps }, 
      m_filter{ ::std::move(filter) }, 
      m_block_builder{ *m_deps, get_compressor(*m_deps->opt(), "zstd") },
      m_file{ ::std::move(file) }
{
}

koios::task<bool> sstable_builder::add(
    const sequenced_key& key, const kv_user_value& value)
{
    assert(!was_finish());
    assert(m_filter != nullptr);
    if (::std::string_view uk = key.user_key(); uk != m_last_uk)
    {
        m_filter->append_new_filter(uk, m_filter_rep);
        m_last_uk = uk;
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
    meta_builder.add({0, "bloomfilter"}, m_filter_rep);
    m_block_builder = ::std::move(meta_builder);

    const mbo_t mbo = m_bytes_appended_to_file;
    co_await flush_current_block(false);
    ::std::string mbo_and_magic_number_buffer;
    append_encode_int_to<sizeof(mbo)>(mbo, mbo_and_magic_number_buffer);
    append_encode_int_to<sizeof(magic_number)>(magic_number, mbo_and_magic_number_buffer);
    co_await m_file->append(::std::as_bytes(::std::span{mbo_and_magic_number_buffer}));
    co_await m_file->close();
    co_return true;
}

} // namespace frenzykv
