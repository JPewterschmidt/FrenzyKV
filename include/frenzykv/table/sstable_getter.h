#ifndef FRENZYKV_SSTABLE_GETTER_H
#define FRENZYKV_SSTABLE_GETTER_H

#include <memory>

#include "koios/task.h"

#include "frenzykv/util/file_guard.h"

#include "frenzykv/table/sstable.h"

namespace frenzykv
{

class sstable_getter
{
public:
    virtual ~sstable_getter() noexcept {}
    virtual koios::task<::std::shared_ptr<sstable>> get(file_guard fg) const = 0;
};

} // namespace frenzykv

#endif
