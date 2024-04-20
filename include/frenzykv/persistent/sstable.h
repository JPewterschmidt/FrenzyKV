#ifndef FRENZYKV_PERSISTENT_SSTABLE_H
#define FRENZYKV_PERSISTENT_SSTABLE_H

#include <string>
#include <memory>

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/persistent/block.h"

#include "frenzykv/writable.h"

namespace frenzykv
{

class sstable_builder
{
public:
    sstable_builder(const kvdb_deps& deps, 
                    ::std::unique_ptr<filter_policy> filter, 
                    ::std::unique_ptr<seq_writable> file);
    
    koios::task<bool> add(const sequenced_key& key, const kv_user_value& value);
    koios::task<bool> add(const kv_entry& kv) { return add(kv.key(), kv.value()); }
    bool was_finish() const noexcept { return m_finish; }
    koios::task<bool> finish();

private:
    koios::task<bool> flush_current_block(bool need_flush = true);

private:
    bool m_finish{};
    const kvdb_deps* m_deps;
    ::std::unique_ptr<filter_policy> m_filter{};
    ::std::string_view m_last_uk{};
    ::std::string m_filter_rep{};
    block_builder m_block_builder;
    ::std::unique_ptr<seq_writable> m_file;
};

} // namespace frenzykv

#endif
