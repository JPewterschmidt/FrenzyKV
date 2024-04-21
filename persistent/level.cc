#include "frenzykv/persistent/level.h"
#include <format>

namespace frenzykv
{

static ::std::string name_a_file(level_t l, file_id_t id)
{
    return ::std::format("frzkv-l{}-{}.frzkv", l, id);
}

file_id_t level::allocate_file_id()
{
    file_id_t result{};
    if (!m_id_recycled.try_dequeue(result))
    {
        result = m_latest_unused_id.fetch_add(1);
    }

    return result;
}

file_id_t level::create_file(level_t level)
{
    file_id_t id = allocate_file_id();
    auto name = name_a_file(level, id);
    
    // TODO

    return id;
}

void level::delete_file(level_t level, file_id_t id)
{
    // TODO
    
    // Recycle file id;
    m_id_recycled.enqueue(id);
}

size_t level::serialize_to(bspan dst) const noexcept
{
    // TODO
}

bool level::parse_from(const_bspan src) noexcept
{
    // TODO
}

} // namespace frenzykv
