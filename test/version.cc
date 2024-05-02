#include "gtest/gtest.h"

#include "koios/task.h"

#include "frenzykv/db/version.h"
#include "frenzykv/db/version_descriptor.h"

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
        co_await new_version(version_delta{}.add_new_files(::std::vector<file_guard>(5)));
        co_await new_version(version_delta{}.add_new_files(::std::vector<file_guard>(2)));
        co_return co_await m_center.size() == 2;
    }

    void reset() noexcept { m_center = {}; }

    koios::task<bool> test_gc(size_t scale = 3)
    {
        for (size_t i{}; i < scale; ++i)
        {
            co_await new_version(version_delta{}.add_new_files(::std::vector<file_guard>(5)));
        }
        co_await new_version(version_delta{}.add_new_files(::std::vector<file_guard>(2)));

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

TEST(version_descriptor_test, name_length)
{
    const auto name1 = get_version_descriptor_name();
    const auto name2 = get_version_descriptor_name();
    ASSERT_EQ(name1.size(), name2.size())  // 52
        << " name1 sz = " << name1.size()
        << " name2 sz = " << name2.size();
}
