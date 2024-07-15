// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_PROTO_PARSE_FROM_INNER_BUFFER_H
#define FRENZYKV_PROTO_PARSE_FROM_INNER_BUFFER_H

#include <optional>
#include "frenzykv/inner_buffer.h"

namespace frenzykv
{
    template<typename ProtoMsg>
    ::std::optional<ProtoMsg> parse_from(const buffer<>& buf) noexcept
    {
        ProtoMsg ret;
        auto readable = buf.valid_span();
        if (ret.ParseFromArray(readable.data(), (int)readable.size_bytes()))
            return ret;
        return {};
    }

    template<typename ProtoMsg>
    ::std::optional<ProtoMsg> parse_from(auto spanlike) noexcept
    {
        ProtoMsg ret;
        if (ret.ParseFromArray(spanlike.data(), (int)spanlike.size_bytes()))
            return ret;
        return {};
    }

    template<typename ProtoMsg>
    ::std::optional<ProtoMsg> parse_from(const ::std::string& str) noexcept
    {
        ProtoMsg ret;
        if (ret.ParseFromString(str))
            return ret;
        return {};
    }

    template<typename ProtoMsg>
    ::std::optional<ProtoMsg> parse_from(::std::string_view str) noexcept
    {
        ProtoMsg ret;
        if (ret.ParseFromArray(str.data(), (int)str.size()))
            return ret;
        return {};
    }
    
} // namespace frenzykv

#endif
