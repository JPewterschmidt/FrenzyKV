#ifndef FRENZYKV_SSTABLE_GETTER_FROM_CACHE_H
#define FRENZYKV_SSTABLE_GETTER_FROM_CACHE_H

#include "frenzykv/util/sstable_getter.h"
#include "frenzykv/util/table_cache.h"

namespace frenzykv
{

class sstable_getter_from_cache : public sstable_getter
{
public:
    sstable_getter_from_cache(table_cache& cache) noexcept;

    virtual koios::task<::std::shared_ptr<sstable>> get(const file_guard& fg) const override;

private:
    table_cache& m_cache;
};

} // namespace frenzykv

#endif
