#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include "koios/task.h"

#include "frenzykv/readable.h"
#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"

namespace frenzykv
{

class sstable
{
public:
    sstable(const kvdb_deps& deps, 
            ::std::unique_ptr<random_readable> file);
    
private:
    koios::task<bool> parse_meta_data();

private:
    ::std::unique_ptr<random_readable> m_file;
    ::std::string m_filter_rep;
    ::std::unique_ptr<filter_policy> m_filter;
};

} // namespace frenzykv

#endif
