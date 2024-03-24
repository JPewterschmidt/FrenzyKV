#include "gtest/gtest.h"
#include "frenzykv/write_batch.h"
#include "google/protobuf/util/message_differencer.h"

using namespace frenzykv;

TEST(write_batch, basic)
{
    const char* kv = "keyvalue";
    ::std::span<const char> k{ kv, 3 };
    ::std::span<const char> v{ kv + 3, 5 };

    write_batch b{as_bytes(k), as_bytes(v)};
    ASSERT_EQ(b.count(), 1);

    ::std::array<::std::byte, 100> buffer{};
    b.serialize_to_array(buffer);
    entry_pbrep example_entry, entry_from_buffer;

    example_entry.mutable_key()->set_seq_number(0);
    example_entry.mutable_key()->set_user_key(k.data(), k.size());
    example_entry.set_value(v.data(), v.size());

    entry_from_buffer.ParseFromArray(buffer.data(), buffer.size());
    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(entry_from_buffer, example_entry));
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
    b.serialize_to_array(buffer);

    entry_pbrep example_entry, entry_from_buffer;

    example_entry.mutable_key()->set_seq_number(0);
    example_entry.mutable_key()->set_user_key(k.data(), k.size());
    example_entry.clear_value();

    entry_from_buffer.ParseFromArray(buffer.data(), buffer.size());
    ASSERT_TRUE(google::protobuf::util::MessageDifferencer::Equals(entry_from_buffer, example_entry));
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
    
    ::std::array<::std::byte, 100> buffer{};
    ASSERT_EQ(b.serialize_to_array(buffer), b.serialized_size());
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

    write_batch b3;
    b3.write(b2); // copy
    ASSERT_EQ(b3.count(), b2.count());
    ASSERT_FALSE(b2.empty());
}
