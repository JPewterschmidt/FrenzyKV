#include <format>
#include <cassert>

#include "frenzykv/env.h"
#include "frenzykv/persistent/level.h"

namespace frenzykv
{

static ::std::string name_a_file(level_t l, file_id_t id)
{
    return ::std::format("frzkv-l{}-{}.frzkv", l, id);
}

koios::task<file_id_t> level::allocate_file_id()
{
    assert(working());
    file_id_t result{};
    if (!m_id_recycled.try_dequeue(result))
    {
        result = m_latest_unused_id.fetch_add(1);
    }

    return result;
}

koios::task<file_id_t> level::create_file(level_t level)
{
    assert(working());
    file_id_t id = allocate_file_id();
    auto name = name_a_file(level, id);
    
    co_return id;
}

koios::task<> level::delete_file(level_t level, file_id_t id)
{
    assert(working());
    auto lk = co_await m_mutex.acquire();
    co_await uring::unlink(sstables_dir()/m_id_name.at(id));

    // Recycle file id;
    m_id_recycled.enqueue(id);
}

koios::task<> level::finish() const noexcept
{
    int val = m_working.load();
    if (val == 2) co_return;
    else if (val == 0) 
        throw koios::exception{"level: you can not finish a unstarted level object"};

    auto lk = co_await m_mutex.acqiure();
    
    // TODO

    m_working = false;
    co_return;
}

koios::task<> level::start() const noexcept
{
    int val = m_working.load();
    if (val == 1) co_return;
    else if (val == 2) 
        throw koios::exception{"level: you can not start a finished level object"};

    auto lk = co_await m_mutex.acquire();

    // TODO

    m_working.fetch_add();
    co_return;
}

size_t level::serialize_to(bspan dst) const noexcept
{
    // TODO
    return {};
}

bool level::parse_from(const_bspan src) noexcept
{
    // TODO
    return {};
}

bool level::working() const noexcept
{
    return m_working.load() == 1;
}

} // namespace frenzykv
