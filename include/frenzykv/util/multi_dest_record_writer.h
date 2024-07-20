// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_RECORD_WRITER_H
#define FRENZYKV_RECORD_WRITER_H

#include "frenzykv/concepts.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/util/nop_record_writer.h"
#include "frenzykv/util/record_writer_wrapper.h"
#include "koios/task.h"
#include <tuple>

namespace frenzykv
{

template<typename WritersTuple = ::std::tuple<nop_record_writer>>
class multi_dest_record_writer
{
public:
    constexpr multi_dest_record_writer() noexcept = default;
    multi_dest_record_writer(WritersTuple writers) noexcept
        : m_writers{ ::std::move(writers) }
    {
    }

    multi_dest_record_writer(multi_dest_record_writer&&) noexcept = default;
    multi_dest_record_writer& operator = (multi_dest_record_writer&&) noexcept = default;

    template<is_record_writer NewWriter>
    auto new_with(NewWriter&& nw) &&
    {
        auto t = ::std::tuple_cat(
            ::std::move(m_writers), 
            ::std::tuple{ ::std::forward<NewWriter>(nw) }
        );
        return multi_dest_record_writer<::std::remove_reference_t<decltype(t)>>{ ::std::move(t) };
    }

    koios::task<> write(const write_batch& batch)
    {
        auto batch_and_writers = ::std::tuple_cat(::std::tuple{ ::std::cref(batch) }, ::std::move(m_writers));
        co_await ::std::apply([](const write_batch& batch, auto&& ... writer_ptrs) -> koios::task<> {
            (co_await writer_ptrs.write(batch), ...);
        }, ::std::move(batch_and_writers));
    }

private:
    WritersTuple m_writers{};   
};

} // namespace frenzykv

#endif
