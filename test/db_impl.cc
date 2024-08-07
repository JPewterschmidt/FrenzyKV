// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <filesystem>
#include "gtest/gtest.h"
#include "frenzykv/db/db_impl.h"

namespace fs = ::std::filesystem;

namespace 
{

using namespace frenzykv;

class db_impl_test : public ::testing::Test
{
public:
    db_impl_test()
        : m_db{ "unittest_frenzykv", get_global_options() }
    {
    }

private:
    db_impl m_db;
};

} // annoymous namespace

TEST_F(db_impl_test, basic)
{
    ::std::error_code ec;
    ASSERT_TRUE(fs::exists(sstables_path(), ec));
    ASSERT_TRUE(fs::exists(prewrite_log_path(), ec));
    ASSERT_TRUE(fs::exists(system_log_path(), ec));
    ASSERT_TRUE(fs::exists(config_path(), ec));
}
