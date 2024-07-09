// Copyright (c) 2012 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.
//
// https://github.com/google/leveldb/blob/main/util/bloom_test.cc

// Tests for bloom filter are copied and modified from Google's LevelDB implementation. 
// Thank you Google.

#include "gtest/gtest.h"
#include "frenzykv/db/filter.h"

#include <string>
#include <vector>
#include <ranges>
#include <string_view>

using namespace frenzykv;

namespace
{

const_bspan make_dummy_key(int i, bspan buffer)
{
    auto writable_buf = ::std::as_writable_bytes(buffer);

    // Recent clang and gcc optimize this to a single mov / str instruction.
    writable_buf[0] = static_cast<::std::byte>(i);
    writable_buf[1] = static_cast<::std::byte>(i >> 8);
    writable_buf[2] = static_cast<::std::byte>(i >> 16);
    writable_buf[3] = static_cast<::std::byte>(i >> 24);
    return { buffer.data(), sizeof(uint32_t) };
}

class bloom_test : public testing::Test
{
public:
    bloom_test()
        : m_policy{ make_bloom_filter(12) }
    {
    }

    void reset()
    {
        m_filter.clear();
        m_keys.clear();
    }

    void add(::std::string_view s) { m_keys.emplace_back(s); }

    void add(const_bspan key) 
    { 
        ::std::string_view str{ 
            reinterpret_cast<const char*>(key.data()), 
            key.size() 
        };
        m_keys.emplace_back(str); 
    }

    void build()
    {
        namespace rv = ::std::ranges::views;
        ::std::vector<const_bspan> vec_keyspans;
        for (const auto& key : m_keys)
            vec_keyspans.emplace_back(::std::as_bytes(::std::span{key}));
        m_filter.clear();
        m_policy->append_new_filter(vec_keyspans, m_filter);
        m_keys.clear();
    }

    size_t filter_size() const noexcept { return m_filter.size(); }

    bool matches(::std::string_view s)
    {
        ::std::span temp(s);
        return matches(::std::as_bytes(temp));
    }
    
    bool matches(const_bspan s)
    {
        if (!m_keys.empty())
            build();
        return m_policy->may_match(s, m_filter);
    }

    double false_positive_rate()
    {
        ::std::array<::std::byte, sizeof(int)> buffer{};
        int result{};
        for (int i{}; i < 100000; ++i)
        {
            if (matches(make_dummy_key(i + 1000000000, buffer)))
                ++result;
        }

        return result / 100000.0;
    }

private:
    const ::std::unique_ptr<filter_policy> m_policy{};
    ::std::string m_filter{};
    ::std::vector<::std::string> m_keys{};
};

} // annoymous namespace

TEST_F(bloom_test, empty_filter)
{
    ASSERT_TRUE(!this->matches("fuck you"));
    ASSERT_TRUE(!this->matches("final project"));
}

TEST_F(bloom_test, small_filter)
{
    reset();
    add("Thanks");
    add("Google");
    build();
    ASSERT_TRUE(matches("Thanks"));
    ASSERT_TRUE(matches("You"));
    ASSERT_TRUE(!matches("Hello"));
    ASSERT_TRUE(!matches("World"));
}

TEST_F(bloom_test, var_length_filter)
{
    ::std::array<::std::byte, sizeof(int)> buffer{};
    int mediocre_filters{};
    int good_filters{};

    auto next_len = [](int length){ 
        /**/ if (length < 10)       length += 1;
        else if (length < 100)      length += 10; 
        else if (length < 1000)     length += 100; 
        else if (length < 10000)    length += 1000; 
        else                        length += 10000;
        return length;
    };
        
    constexpr int start_length = 23;
    for (int length{start_length}; length < 10000; length = next_len(length))
    {
        reset();
        for (int i{start_length}; i < length; ++i)
        {
            add(make_dummy_key(i, buffer));
        }
        build();

        ASSERT_LE(filter_size(), static_cast<size_t>(length * 1.5 + 40))
            << "filter_size(): " << filter_size()
            << ", length: "      << length
            << ", filter_size() / length: " 
            << (double)filter_size() / (double)length;

        for (int i{start_length}; i < length; ++i)
        {
            ASSERT_TRUE(matches(make_dummy_key(i, buffer)))
                << "length " << length << "; key" << i;
        }

        double rate = false_positive_rate();
        ASSERT_LE(rate, 0.02);
        if (rate > 0.0125)
            ++mediocre_filters;
        else ++good_filters;
        ASSERT_LE(mediocre_filters, good_filters / 5) 
            << "good filters: " << good_filters
            << ", mediocre_filters: " << mediocre_filters
            << ", length: " << length;
    }
}
