#include "gtest/gtest.h"

#include "koios/task.h"

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

    void reset() noexcept { m_center = {}; }

    koios::task<bool> test_gc(size_t scale = 3)
    {
        for (size_t i{}; i < scale; ++i)
        {
            co_await new_version(version_delta{}.add_new_files(::std::vector{1, 2, 3, 4, 5}));
        }
        co_await new_version(version_delta{}.add_new_files(::std::vector{6, 7}));

        const size_t oldsize = co_await size();

        size_t gced{};
        co_await m_center.GC_with(
            [&gced]([[maybe_unused]] auto&& item) -> koios::task<> { 
                ++gced; 
                co_return; 
            }
        );
        
        const bool result = gced == scale && co_await size() == 1 && oldsize == scale + 1;
        co_return result;
    }

protected:
    version_center m_center;
};

} // annoymous namespace

TEST_F(version_test, basic)
{
    reset();
    ASSERT_TRUE(test_basic().result());
}

TEST_F(version_test, GC)
{
    reset();
    ASSERT_TRUE(test_gc().result());
}