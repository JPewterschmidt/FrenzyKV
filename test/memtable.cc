#include "gtest/gtest.h"
#include "frenzykv/db/memtable.h"
#include "frenzykv/kvdb_deps.h"

using namespace frenzykv;

namespace
{

koios::task<bool> init_ok(memtable& m)
{
    co_return {
           co_await m.count() == 0
        && co_await m.bound_size_bytes() != 0
        && co_await m.full() == false
        && co_await m.size_bytes() == 0
    };
}

write_batch make_batch()
{
    write_batch result;
    result.write("abc1", ::std::string(1024, 'x'));
    result.write("abc2", ::std::string(1024, 'x'));
    result.write("abc3", ::std::string(1024, 'x'));
    result.write("abc4", ::std::string(1024, 'x'));
    result.set_first_sequence_num(0);
    return result;
}

koios::task<bool> insert_test(memtable& m)
{
    auto b = make_batch();
    write_batch b2;
    b2.write("abc1", ::std::string());
    b2.set_first_sequence_num(100);
    co_await m.insert(b);
    co_await m.insert(::std::move(b2));
    co_return co_await m.count() == b.count() + b2.count()
        && co_await m.size_bytes() == b.serialized_size() + b2.serialized_size();
}

koios::task<bool> get_test(memtable& m)
{
    seq_key k;
    k.set_user_key("abc1");
    k.set_seq_number(99);
    auto kv_opt = co_await m.get(k);
    if (!kv_opt) co_return false;
    const auto& kv = kv_opt.value();
    co_return kv.key().seq_number() == 100 && kv.value().size() == 0;
}

} // annoymous namespace 

TEST(memtable, basic)
{
    kvdb_deps deps{};
    memtable m{ deps };
    ASSERT_TRUE(init_ok(m).result());
    ASSERT_TRUE(insert_test(m).result());
    ASSERT_TRUE(get_test(m).result());
}
