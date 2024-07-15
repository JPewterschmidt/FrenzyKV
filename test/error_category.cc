// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "gtest/gtest.h"
#include "frenzykv/error_category.h"
#include <system_error>

using namespace frenzykv;

TEST(error_category, basic)
{
    ::std::error_code ec{ FRZ_KVDB_OK, kvdb_category() };
    ASSERT_TRUE(ec.message().contains("Ok"));
    ec = { FRZ_KVDB_NOTFOUND, kvdb_category() };
    ASSERT_TRUE(ec.message().contains("Not Found"));
}
