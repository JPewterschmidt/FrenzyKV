#include "gtest/gtest.h"

#include "frenzykv/db/version.h"

namespace 
{

using namespace frenzykv;

class version_test : public ::testing::Test
{
public:
    koios::task<> new_version(version_delta delta)
    {
        co_await m_center.add_new_version() += delta;
    }

    koios::task<size_t> size() { return m_center.size(); }

    koios::task<bool> test_basic()
    {
        co_await new_version(version_delta{}.add_new_files(::std::vector{1, 2, 3, 4, 5}));
        co_await new_version(version_delta{}.add_new_files(::std::vector{6, 7}));
        co_return co_await m_center.size() == 2;
    }

protected:
    version_center m_center;
};

} // annoymous namespace

TEST_F(version_test, basic)
{
    ASSERT_TRUE(test_basic().result());
}
