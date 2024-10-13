// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include <filesystem>
#include "gtest/gtest.h"
#include "frenzykv/db/db_local.h"

namespace fs = ::std::filesystem;

namespace 
{

using namespace frenzykv;

class db_local_test : public ::testing::Test
{
public:
    db_local_test()
        : m_db{ "unittest_frenzykv", get_global_options() }
    {
    }

    db_local m_db;
};

} // annoymous namespace

TEST_F(db_local_test, basic)
{
    ::std::error_code ec;
    auto env = m_db.env();
    ASSERT_TRUE(fs::exists(env->sstables_path(), ec));
    ASSERT_TRUE(fs::exists(env->write_ahead_log_path(), ec));
    ASSERT_TRUE(fs::exists(env->system_log_path(), ec));
    ASSERT_TRUE(fs::exists(env->config_path(), ec));
}
