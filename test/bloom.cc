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
    buffer[0] = static_cast<::std::byte>(i);
    buffer[1] = static_cast<::std::byte>(i >> 8);
    buffer[2] = static_cast<::std::byte>(i >> 16);
    buffer[3] = static_cast<::std::byte>(i >> 24);
    return { buffer.data(), sizeof(uint32_t) };
}

class bloom_test : public testing::Test
{
public:
    bloom_test()
        : m_policy{ make_bloom_filter(10) }
    {
    }

    void reset()
    {
        m_filter.clear();
        m_keys.clear();
    }

    void add(::std::string_view s) { m_keys.emplace_back(s); }
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
        if (m_keys.empty())
            build();
        return m_policy->may_match(s, m_filter);
    }

    double false_positive_rate()
    {
        ::std::array<::std::byte, sizeof(int)> buffer{};
        int result{};
        for (int i{}; i < 10000; ++i)
        {
            if (matches(make_dummy_key(i + 1000000000, buffer)))
                ++result;
        }

        return result / 10000.0;
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
