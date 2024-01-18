#include "gtest/gtest.h"
#include "frenzykv/error_category.h"
#include <system_error>

using namespace frenzykv;

TEST(error_category, basic)
{
    ::std::error_code ec{ FRZ_KVDB_OK, kvdb_category() };
    ASSERT_EQ(ec.message(), "Ok");
    ec = { FRZ_KVDB_NOTFOUND, kvdb_category() };
    ASSERT_EQ(ec.message(), "Not Found");
}
