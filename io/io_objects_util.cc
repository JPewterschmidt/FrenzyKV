#include "frenzykv/io/io_objects_util.h"

namespace frenzykv
{

koios::task<::std::unique_ptr<in_mem_rw>>
disk_to_mem(::std::unique_ptr<random_readable> file)
{
    const uintmax_t fsz = file->file_size();
    if (fsz > 4ull * 1024ull * 1024ull * 1024ull) // 4GB
    {
        throw koios::exception{ "disk_to_mem: file too big (> 4G). I don't want to load it into memory." };
    }

    auto result = ::std::make_unique<in_mem_rw>(fsz);
    auto wsp = result->writable_span();
    
    size_t offset{};
    while (offset < fsz)
    {
        size_t readed = co_await file->read(wsp, offset);
        co_await result->commit(readed);
        offset += readed;
    }
    
    co_return result;
}

} // namespace frenzykv
