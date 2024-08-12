// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "gtest/gtest.h"
#include "frenzykv/write_batch.h"
#include "google/protobuf/util/message_differencer.h"

#include <ranges>

using namespace frenzykv;
namespace r = ::std::ranges;
namespace rv = r::views;

TEST(write_batch, basic)
{
    const char* kv = "keyvalue";
    ::std::span<const char> k{ kv, 3 };
    ::std::span<const char> v{ kv + 3, 5 };

    write_batch b{as_bytes(k), as_bytes(v)};
    ASSERT_EQ(b.count(), 1);

    ::std::array<::std::byte, 100> buffer{};
    b.serialize_to(buffer);
    kv_entry example_entry(0, as_bytes(k), as_bytes(v)), entry_from_buffer(buffer);

    ASSERT_EQ(entry_from_buffer, example_entry);
}

TEST(write_batch, remove)
{
    const char* kv = "keyvalue";
    ::std::span<const char> k{ kv, 3 };
    ::std::span<const char> v{ kv + 3, 5 };

    write_batch b{as_bytes(k), as_bytes(v)};
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    ASSERT_EQ(b.count(), 5);
    
    b.remove_from_db(as_bytes(k));
    ASSERT_EQ(b.count(), 1);

    ::std::array<::std::byte, 100> buffer{};
    b.serialize_to(buffer);

    kv_entry example_entry(0, as_bytes(k)), entry_from_buffer(buffer);
    ASSERT_EQ(entry_from_buffer, example_entry);
}

TEST(write_batch, serialized_size)
{
    const char* kv = "keyvalue";
    ::std::span<const char> k{ kv, 3 };
    ::std::span<const char> v{ kv + 3, 5 };

    write_batch b{as_bytes(k), as_bytes(v)};
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    
    ::std::array<::std::byte, 512> buffer{};
    ASSERT_EQ(b.serialize_to(buffer), b.serialized_size());
}

TEST(write_batch, from_another_batch)
{
    const char* kv = "keyvalue";
    ::std::span<const char> k{ kv, 3 };
    ::std::span<const char> v{ kv + 3, 5 };

    write_batch b{as_bytes(k), as_bytes(v)};
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));
    b.write(as_bytes(k), as_bytes(v));

    const size_t old_count = b.count();

    write_batch b2;
    b2.write(::std::move(b));
    ASSERT_EQ(b.count(), 0);
    ASSERT_TRUE(b.empty());
    ASSERT_EQ(b2.count(), old_count);
}

TEST(write_batch, string_param)
{
    ::std::string kv = "keyvalue";
    ::std::string k = kv | rv::take(3) | r::to<::std::string>();
    ::std::string v = kv | rv::drop(3) | r::to<::std::string>();
    
    write_batch b{ k, v };
    b.write(k, v);
    b.write(k, v);
    b.write(k, v);
    b.write(::std::move(k), ::std::move(v));

    ::std::array<::std::byte, 512> buffer{};
    ASSERT_EQ(b.serialize_to(buffer), b.serialized_size());
}
