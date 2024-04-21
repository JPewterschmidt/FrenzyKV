#include "frenzykv/io/io_objects_util.h"

namespace frenzykv
{

koios::task<in_mem_rw>
to_in_mem_rw(random_readable& file)
{
    const uintmax_t fsz = file.file_size();
    if (fsz > 4ull * 1024ull * 1024ull * 1024ull) // 4GB
    {
        throw koios::exception{ "disk_to_mem: file too big (> 4G). I don't want to load it into memory." };
    }

    in_mem_rw result{ fsz };
    auto wsp = result.writable_span();
    
    size_t offset{};
    while (offset < fsz)
    {
        size_t readed = co_await file.read(wsp, offset);
        co_await result.commit(readed);
        offset += readed;
    }
    
    co_return result;
}

} // namespace frenzykv
