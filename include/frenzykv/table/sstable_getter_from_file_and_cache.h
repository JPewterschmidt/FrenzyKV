// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_SSTABLE_GETTER_FROM_FILE_AND_CACHE_H
#define FRENZYKV_SSTABLE_GETTER_FROM_FILE_AND_CACHE_H

#include "frenzykv/table/sstable_getter.h"
#include "frenzykv/table/table_cache.h"

namespace frenzykv
{

class sstable_getter_from_file : public sstable_getter
{
public:
    sstable_getter_from_file(table_cache& cache, const kvdb_deps& deps, filter_policy* filter) noexcept
        : m_cache{ cache }, m_deps{ &deps }, m_filter{ filter }
    {
    }

    virtual koios::task<::std::shared_ptr<sstable>> get(file_guard fg) const override;

private:
    table_cache& m_cache;
    const kvdb_deps* m_deps{};
    filter_policy* m_filter{};
};

} // namespace frenzykv

#endif
