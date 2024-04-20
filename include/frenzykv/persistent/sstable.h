#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include <string>

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/persistent/block.h"

namespace frenzykv
{

class sstable
{
public:
    sstable(const kvdb_deps& deps, ::std::unique_ptr<filter_policy> filter)
        : m_deps{ &deps }, m_filter{ ::std::move(filter) }
    {
    }
    
    

private:
    ::std::string m_storage;
    const kvdb_deps* m_deps;
    ::std::unqiue_ptr<filter_policy> m_filter{};
};

} // namespace frenzykv

#endif
