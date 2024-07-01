#ifndef FRENZYKV_SSTABLE_GETTER_FROM_FILE_H
#define FRENZYKV_SSTABLE_GETTER_FROM_FILE_H

#include "frenzykv/table/sstable_getter.h"

namespace frenzykv
{

class sstable_getter_from_file : public sstable_getter
{
public:
    sstable_getter_from_file(const kvdb_deps& deps, filter_policy* filter) noexcept
        : m_deps{ &deps }, m_filter{ filter }
    {
    }

    virtual koios::task<::std::shared_ptr<sstable>> get(file_guard fg) const override;

private:
    const kvdb_deps* m_deps{};
    filter_policy* m_filter{};
};

} // namespace frenzykv

#endif
