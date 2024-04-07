#include "gtest/gtest.h"
#include "frenzykv/util/multi_dest_record_writer.h"
#include "frenzykv/util/stdout_debug_record_writer.h"

using namespace koios;
using namespace frenzykv;

namespace
{

::std::vector<::std::size_t> order_book{};

template<::std::size_t Num>
struct test_record_writer
{
    koios::task<> write(const write_batch& b)
    {
        s_success = true;
        order_book.push_back(Num);
        co_return;
    }
    static bool s_success;
};

template<> bool test_record_writer<0>::s_success = false;
template<> bool test_record_writer<1>::s_success = false;
template<> bool test_record_writer<2>::s_success = false;

task<bool> test_body()
{
    record_writer_wrapper writer = multi_dest_record_writer{}
        .new_with(test_record_writer<0>{})
        .new_with(test_record_writer<1>{})
        .new_with(test_record_writer<2>{});
    
    write_batch wb{};
    co_await writer.write(wb);
    co_return true;
}

} // namespace {anonymous}

TEST(record_writer, basic)
{
    ASSERT_TRUE(test_body().result());   
    ASSERT_TRUE(test_record_writer<0>::s_success);
    ASSERT_TRUE(test_record_writer<1>::s_success);
    ASSERT_TRUE(test_record_writer<2>::s_success);

    ASSERT_EQ((order_book), (::std::vector<::std::size_t>{ 0,1,2 }));
}
