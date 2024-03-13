#include "util/stdout_debug_record_writer.h"
#include "toolpex/functional.h"

namespace frenzykv
{

size_t stdout_debug_record_writer::s_number = 0;

stdout_debug_record_writer::
stdout_debug_record_writer() noexcept
    : m_number{ s_number++ }
{
}

void 
stdout_debug_record_writer::awaitable::
await_resume() const noexcept
{
    ::std::string temp = toolpex::lazy_string_concater{}
        + "stdout debug writer: " + m_num + " "
        + "batch = " + m_batch->to_string_debug();
    ::std::cout << temp << ::std::endl;
}

} // namespace frenzykv
