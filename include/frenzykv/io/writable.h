// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_WRITABLE_H
#define FRENZYKV_WRITABLE_H

#include "koios/expected.h"
#include "toolpex/move_only.h"

#include <string_view>
#include <ostream>

#include "frenzykv/io/file.h"

namespace frenzykv
{

class seq_writable : public toolpex::move_only, virtual public file
{
public:
    virtual ~seq_writable() noexcept {}

    koios::task<size_t> 
    append(::std::string_view buffer) noexcept 
    {
        ::std::span<const char> temp{ buffer.begin(), buffer.end() };
        co_return co_await append(as_bytes(temp));
    }

    virtual koios::task<size_t> append(::std::span<const ::std::byte> buffer) = 0;
    virtual koios::task<> sync() = 0;
    virtual koios::task<> flush() = 0;
    
    // The following 2 methods support users to get a writable range of memory to write.
    // But the users must inform the actual number of bytes they want to commit.

    /*! \brief  Get a writable range of memory.
     *  \return When success, a non-empty span would return, represents the writable memory 
     *          (typically from a buffer) which may or may not been initialized.
     *          If the underlying facility isn't support buffered IO, then may return a empty span.
     */
    virtual ::std::span<::std::byte> writable_span() = 0;

    /*! \brief  Commits the recent wrote.
     *  \attention This function should be called after your write to the most recently returned span complete.
     *             to inform the underlying IO facility to update the buffer state.
     *             Or you will lose your data.
     *             If you use method `append()`, do NOT call this function, 
     *             or you will insert a gap in the buffer.
     */
    virtual koios::task<> commit(size_t wrote_len) = 0;

    /*! \brief  To get the stream buffer of this object.
     *  To support protobuf.
     *  You can use this to build a temporary `std::ostream` object.
     */
    virtual ::std::streambuf* streambuf() = 0;
};

} // namespace frenzykv

#endif
