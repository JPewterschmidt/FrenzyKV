#include "toolpex/exceptions.h"
#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

koios::task<bool> sstable::parse_meta_data()
{
    // TODO
    toolpex::not_implemented();
    co_return {};
}

} // namespace frenzykv
