#ifndef FRENZYKV_SSTABLE_GETTER_H
#define FRENZYKV_SSTABLE_GETTER_H

#include <memory>

#include "frenzykv/util/file_guard.h"

namespace frenzykv
{

class sstable_getter
{
public:
    virtual ::std::shared_ptr<sstable> get(const file_gaurd& fg) const = 0;
};

} // namespace frenzykv

#endif
