#include "frenzykv/persistent/sstable.h"

namespace frenzykv
{

static constexpr uint32_t magic_number = 0x47d6ddc3;

bool sstable::add(const sequenced_key& key, const kv_user_value& value)
{
    assert(was_finish());
    assert(m_filter != nullptr);
    if (key.user_key() != m_last_uk)
    {
        // TODO: add new key to filter.
    }

    return true;
}

} // namespace frenzykv
