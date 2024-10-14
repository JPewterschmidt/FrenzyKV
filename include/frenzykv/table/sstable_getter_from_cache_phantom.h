// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_SSTABLE_GETTER_FROM_CACHE_PHANTOM_H
#define FRENZYKV_SSTABLE_GETTER_FROM_CACHE_PHANTOM_H

#include "frenzykv/table/sstable_getter.h"
#include "frenzykv/table/table_cache.h"

namespace frenzykv
{

class sstable_getter_from_cache_phantom : public sstable_getter
{
public:
    sstable_getter_from_cache_phantom(table_cache& cache) noexcept;

    virtual koios::task<::std::shared_ptr<sstable>> get(file_guard fg) const override;

private:
    table_cache& m_cache;
};

} // namespace frenzykv

#endif
