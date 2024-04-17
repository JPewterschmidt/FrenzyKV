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
    ASSERT_TRUE(fs::exists(sstables_path()));
    ASSERT_TRUE(fs::exists(prewrite_log_path()));
    ASSERT_TRUE(fs::exists(system_log_path()));
    ASSERT_TRUE(fs::exists(config_path()));
}
