// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <print>

#include "frenzykv/util/stdout_debug_record_writer.h"
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
    ::std::println("stdout debug writer: {} batch = {}", m_num, m_batch->to_string_debug());
}

} // namespace frenzykv
