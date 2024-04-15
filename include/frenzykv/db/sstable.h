#ifndef FRENZYKV_SSTABLE_H
#define FRENZYKV_SSTABLE_H

#include <memory>
#include <optional>
#include "koios/task.h"
#include "frenzykv/readable.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/db/kv_entry.h"

namespace frenzykv
{

class sstable_interface
{
public: 
    virtual 
    koios::task<::std::optional<kv_entry>> 
    get(const sequenced_key& key) const = 0;

    virtual 
    koios::task<bool> 
    write(const write_batch& batch) = 0;
};

} // namespace frenzykv

#endif
