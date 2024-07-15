// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "gtest/gtest.h"
#include "frenzykv/db/kv_entry.h"
#include "frenzykv/util/comp.h"
#include <string>
#include <functional>

using namespace frenzykv;

static auto get_data_sameuk_diffsn()
{
    sequenced_key key1{15, "xxxxxxxxx"}, key2{270, "xxxxxxxxx"};
    // we need make sure the lexicgraphical order are satisfied with this manner.
    sequenced_key key3{271+15, "xxxxxxxxx"}, key4{271+16, "xxxxxxxxx"};
                                 // Thus key3 < key4 is considerable.

    return ::std::tuple{ 
        ::std::move(key1), 
        ::std::move(key2), 
        ::std::move(key3), 
        ::std::move(key4),
    };
}

static auto get_data_diffuk_diffsn()
{
    sequenced_key key1{16, "123"}; 
    sequenced_key key2{8, "12345"};
    sequenced_key key3{100000, "def"};
    sequenced_key key4{9999999, "adbc"};

    return ::std::tuple{ 
        ::std::move(key1), 
        ::std::move(key2), 
        ::std::move(key3), 
        ::std::move(key4),
    };
}

TEST(lexicographical_order, serialized_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.serialize_to(key1_str);
    key2.serialize_to(key2_str);
    key3.serialize_to(key3_str);
    key4.serialize_to(key4_str);

    ASSERT_TRUE(::std::less<::std::string>{}(key1_str, key2_str));
    ASSERT_TRUE(::std::less<::std::string>{}(key1_str, key3_str));
    ASSERT_TRUE(::std::less<::std::string>{}(key3_str, key4_str));
}

TEST(lexicographical_order, before_serializing_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key1, key2));
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key1, key3));
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key3, key4));
}

TEST(lexicographical_order, expect_same_order_sameuk)
{
    auto [key1, key2, key3, key4] = get_data_sameuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.serialize_to(key1_str);
    key2.serialize_to(key2_str);
    key3.serialize_to(key3_str);
    key4.serialize_to(key4_str);
    
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key1, key2), serialized_sequenced_key_less{}(key1_str, key2_str));
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key1, key3), serialized_sequenced_key_less{}(key1_str, key3_str));
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key3, key4), serialized_sequenced_key_less{}(key3_str, key4_str));
}

TEST(lexicographical_order, serialized_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.serialize_to(key1_str);
    key2.serialize_to(key2_str);
    key3.serialize_to(key3_str);
    key4.serialize_to(key4_str);

    ASSERT_TRUE(::std::less<::std::string>{}(key1_str, key2_str));
    ASSERT_TRUE(::std::less<::std::string>{}(key1_str, key3_str));
    ASSERT_TRUE(::std::less<::std::string>{}(key3_str, key4_str));
}

TEST(lexicographical_order, before_serializing_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key1, key2));
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key1, key3));
    ASSERT_TRUE(::std::less<frenzykv::sequenced_key>{}(key3, key4));
}

TEST(lexicographical_order, expect_same_order_diffuk)
{
    auto [key1, key2, key3, key4] = get_data_diffuk_diffsn();
    ::std::string key1_str, key2_str, key3_str, key4_str;

    key1.serialize_to(key1_str);
    key2.serialize_to(key2_str);
    key3.serialize_to(key3_str);
    key4.serialize_to(key4_str);
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key1, key2), serialized_sequenced_key_less{}(key1_str, key2_str));
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key1, key3), serialized_sequenced_key_less{}(key1_str, key3_str));
    ASSERT_EQ(::std::less<frenzykv::sequenced_key>{}(key3, key4), serialized_sequenced_key_less{}(key3_str, key4_str));
}
