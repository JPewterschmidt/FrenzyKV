#include "toolpex/assert.h"
#include "frenzykv/table/sstable_getter_from_file.h"

namespace frenzykv
{

koios::task<::std::shared_ptr<sstable>> sstable_getter_from_file::
get(file_guard fg) const 
{
    auto env = m_deps->env();
    co_return co_await sstable::make(*m_deps, m_filter, co_await fg.open_read(env.get()));
}

} // namespace frenzykv
