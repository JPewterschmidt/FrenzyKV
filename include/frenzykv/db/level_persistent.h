#ifndef FRENZYKV_LEVEL_PERSISTENT_H
#define FRENZYKV_LEVEL_PERSISTENT_H

#include <optional>
#include "koios/task.h"
#include "entry_pbrep.pb.h"
#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

class level_persistent
{
public:
    level_persistent(const kvdb_deps& deps) noexcept
        : m_deps{ &deps }
    {
    }

    koios::task<::std::optional<entry_pbrep>> get(const seq_key& key)
    {
        co_return {};
    }

    koios::task<> write(imm_memtable table)
    {
        co_return;                     
    }

    koios::task<> level_transformation();
    koios::task<> do_GC();

private:
    const kvdb_deps* m_deps{};
};

} // namespace frenzykv

#endif
