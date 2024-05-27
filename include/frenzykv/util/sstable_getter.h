#ifndef FRENZYKV_SSTABLE_GETTER_H
#define FRENZYKV_SSTABLE_GETTER_H

#include <memory>

#include "koios/task.h"

#include "frenzykv/util/file_guard.h"
#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

class sstable_getter
{
public:
    virtual koios::task<::std::shared_ptr<sstable>> get(const file_guard& fg) const = 0;
};

} // namespace frenzykv

#endif
