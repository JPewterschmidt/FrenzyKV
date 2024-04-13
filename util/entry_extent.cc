#include "frenzykv/util/entry_extent.h"

namespace frenzykv
{

bool is_entry_regular(const entry_pbrep& entry)
{
    assert(entry.IsInitialized());
    return entry.has_value();
}

bool is_entry_deleting(const entry_pbrep& entry)
{
    assert(entry.IsInitialized());
    return !entry.has_value();
}

entry_pbrep* 
construct_regular_entry(
    entry_pbrep* entry, 
    const_bspan key, 
    sequence_number_t seq, 
    const_bspan value)
{
    assert(entry);

    entry->mutable_key()->set_user_key(key.data(), key.size());
    entry->mutable_key()->set_seq_number(seq);
    entry->mutable_value()->set_deleted(false);
    entry->mutable_value()->set_value(value.data(), value.size());

    return entry;
}

entry_pbrep* 
construct_deleting_entry(
    entry_pbrep* entry, 
    const_bspan key, 
    sequence_number_t seq)
{
    assert(entry);

    entry->mutable_key()->set_user_key(key.data(), key.size());
    entry->mutable_key()->set_seq_number(seq);
    entry->mutable_value()->set_deleted(true);

    return entry;
}

} // namespace frenzykv
