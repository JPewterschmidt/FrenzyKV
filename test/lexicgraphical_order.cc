#include "gtest/gtest.h"
#include "entry_pbrep.pb.h"
#include <string>
#include <functional>
#include "util/key_cmp.h"

using namespace frenzykv;

static auto get_data_sameuk_diffsn()
{
    seq_key key1, key2, key3, key4;

    key1.set_user_key("xxxxxxxxx");
    key1.set_seq_number(0);
    key2.set_user_key("xxxxxxxxx");
    key2.set_seq_number(1);
    key3.set_user_key("xxxxxxxxx");
    key3.set_seq_number(10000000);
    key4.set_user_key("xxxxxxxxx");
    key4.set_seq_number(99999999);

    return ::std::tuple{ 
        ::std::move(key1), 
        ::std::move(key2), 
        ::std::move(key3), 
        ::std::move(key4),
    };
}

static auto get_data_diffuk_diffsn()
{
    seq_key key1, key2, key3, key4;

    key1.set_user_key("123");
    key1.set_seq_number(100);
    key2.set_user_key("12345");
    key2.set_seq_number(99);
    key3.set_user_key("def");
    key3.set_seq_number(100000000);
    key4.set_user_key("abcd");
    key4.set_seq_number(99999999999);

    return ::std::tuple{ 
        ::std::move(key1), 
        ::std::move(key2), 
        ::std::move(key3), 
        ::std::move(key4),
    };
}

TEST(lexicgraphical_order, serialized_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.SerializeToString(&key1_str);
    key2.SerializeToString(&key2_str);
    key3.SerializeToString(&key3_str);
    key4.SerializeToString(&key4_str);

    ASSERT_TRUE(::std::less{}(key1_str, key2_str));
    ASSERT_TRUE(::std::less{}(key1_str, key3_str));
    ASSERT_TRUE(::std::less{}(key3_str, key4_str));
}

TEST(lexicgraphical_order, before_serializing_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ASSERT_TRUE(seq_key_less{}(key1, key2));
    ASSERT_TRUE(seq_key_less{}(key1, key3));
    ASSERT_TRUE(seq_key_less{}(key3, key4));
}

TEST(lexicgraphical_order, expect_same_order_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ASSERT_EQ(seq_key_less{}(key1, key2), serialized_seq_key_less{}(key1, key2));
    ASSERT_EQ(seq_key_less{}(key1, key3), serialized_seq_key_less{}(key1, key3));
    ASSERT_EQ(seq_key_less{}(key3, key4), serialized_seq_key_less{}(key3, key4));
}

TEST(lexicgraphical_order, serialized_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.SerializeToString(&key1_str);
    key2.SerializeToString(&key2_str);
    key3.SerializeToString(&key3_str);
    key4.SerializeToString(&key4_str);

    ASSERT_TRUE(::std::less{}(key1_str, key2_str));
    ASSERT_TRUE(::std::less{}(key1_str, key3_str));
    ASSERT_TRUE(::std::less{}(key3_str, key4_str));
}

TEST(lexicgraphical_order, before_serializing_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ASSERT_TRUE(seq_key_less{}(key1, key2));
    ASSERT_TRUE(seq_key_less{}(key1, key3));
    ASSERT_TRUE(seq_key_less{}(key3, key4));
}

TEST(lexicgraphical_order, expect_same_order_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ASSERT_EQ(seq_key_less{}(key1, key2), serialized_seq_key_less{}(key1, key2));
    ASSERT_EQ(seq_key_less{}(key1, key3), serialized_seq_key_less{}(key1, key3));
    ASSERT_EQ(seq_key_less{}(key3, key4), serialized_seq_key_less{}(key3, key4));
}
