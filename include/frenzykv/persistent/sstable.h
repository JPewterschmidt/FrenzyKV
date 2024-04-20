#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include <string>
#include <memory>

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/persistent/block.h"

namespace frenzykv
{

class sstable
{
public:
    sstable(const kvdb_deps& deps, ::std::unique_ptr<filter_policy> filter)
        : m_deps{ &deps }, m_filter{ ::std::move(filter) }
    {
    }
    
    bool add(const sequenced_key& key, const kv_user_value& value);
    bool add(const kv_entry& kv) { return add(kv.key(), kv.value()); }
    bool was_finish() const noexcept { return m_finish; }
    ::std::string finish();

private:
    ::std::string m_storage;
    bool m_finish{};
    const kvdb_deps* m_deps;
    ::std::unique_ptr<filter_policy> m_filter{};
    ::std::string_view m_last_uk{};
    ::std::string m_filter_rep{};
};

} // namespace frenzykv

#endif
