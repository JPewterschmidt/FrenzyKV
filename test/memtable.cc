#include "gtest/gtest.h"
#include "frenzykv/db/memtable.h"
#include "frenzykv/kvdb_deps.h"

using namespace frenzykv;

namespace
{

static kvdb_deps g_deps{};

write_batch make_batch()
{
    write_batch result;
    result.write("abc1", ::std::string(1024, 'x'));
    result.write("abc2", ::std::string(1024, 'x'));
    result.write("abc3", ::std::string(1024, 'x'));
    result.set_first_sequence_num(0);
    return result;
}

class memtable_test : public ::testing::Test
{
public:
    memtable_test()
        : m_mem{ g_deps }
    {
    }

    void reset()
    {
        m_mem = memtable{ g_deps };
    }

    koios::task<bool> init_ok()
    {
        co_return {
               co_await m_mem.count() == 0
            && co_await m_mem.bound_size_bytes() != 0
            && co_await m_mem.full() == false
            && co_await m_mem.size_bytes() == 0
        };
    }

    koios::task<bool> insert_test(sequence_number_t seq_number)
    {
        auto b = make_batch();
        write_batch b2;
        b2.write("abc1", ::std::string());
        b2.set_first_sequence_num(seq_number);
        
        const size_t bcount = b.count(), b2count = b2.count();
        const size_t bss = b.serialized_size(), b2ss = b2.serialized_size();

        co_await m_mem.insert(b);
        co_await m_mem.insert(::std::move(b2));
        co_return co_await m_mem.count() == bcount + b2count 
            && co_await m_mem.size_bytes() == bss + b2ss;
    }

    koios::task<bool> get_test(sequence_number_t seq_number)
    {
        sequenced_key k(seq_number, "abc1");
        auto result_opt = co_await m_mem.get(k);
        if (!result_opt) co_return false;
        const auto& result = result_opt.value();
        co_return result.key().sequence_number() >= seq_number && result.value().value().size() == 0;
    }

    koios::task<bool> delete_test()
    {
        reset();
        co_await insert_test(100);
        write_batch b;
        b.remove_from_db("abc1");
        b.set_first_sequence_num(100);
        co_await m_mem.insert(::std::move(b));
        sequenced_key k(100, "abc1");

        co_return !(co_await m_mem.get(k)).has_value();
    }

private:
    memtable m_mem;
};

} // annoymous namespace 

TEST_F(memtable_test, init)
{
    reset();
    ASSERT_TRUE(init_ok().result());
}

TEST_F(memtable_test, insert)
{
    reset();
    ASSERT_TRUE(insert_test(100).result());
}

TEST_F(memtable_test, get)
{
    reset();
    (void)insert_test(100).result();
    ASSERT_TRUE(get_test(99).result());
}

TEST_F(memtable_test, delete_test)
{
    reset();
    ASSERT_TRUE(delete_test().result());
}
