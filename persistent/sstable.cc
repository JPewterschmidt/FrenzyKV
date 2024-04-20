#include "toolpex/exceptions.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace frenzykv
{

koios::task<bool> sstable::parse_meta_data()
{
    const uintmax_t filesz = m_file->file_size();
    ::std::array<::std::byte, sizeof(mbo_t) + sizeof(mgn_t)> buffer{};
    co_await m_file->read(buffer, filesz);
    mbo_t mbo{};
    mgn_t magic_num{};
    ::std::memcpy(&mbo, buffer.data(), sizeof(mbo));
    ::std::memcpy(&magic_num, buffer.data(), sizeof(magic_num));

    // file integrity check
    if (magic_number_value() != magic_num)
    {
        co_return false;
    }
    
    // TODO
    
    
    co_return {};
}

} // namespace frenzykv
