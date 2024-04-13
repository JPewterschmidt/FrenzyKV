#include "gtest/gtest.h"
#include "frenzykv/util/hash.h"
#include "frenzykv/util/comp.h"
#include <string>

using namespace frenzykv;

class hash_test_executor
{
public:
    hash_test_executor(binary_hash_interface& h) noexcept : m_h{ h } { }

    bool is_state_less() const noexcept
    {
        ::std::array<char, 5> buf1{ "1234" };
        ::std::array<char, 5> buf2{ "1234" };

        return (m_h(buf1) == m_h(buf2))
            && (m_h(buf1) == m_h(buf2))
            && (m_h.hash(buf1) == m_h.hash(buf2));
    }

private:
    binary_hash_interface& m_h;
};

class comp_test_executor
{
public:
    comp_test_executor(binary_comparator_interface& c) noexcept : m_c{ c } { }

    bool is_state_less() const noexcept
    {
        ::std::array<char, 11> buf1{ "poi1234abc" };
        ::std::array<char, 11> buf2{ "mnb1234def" };
        
        const auto ret1 = m_c(::std::span{buf1}.subspan(3, 4), ::std::span{buf2}.subspan(3, 4));
        const auto ret2 = m_c(::std::span{buf1}.subspan(3, 4), ::std::span{buf2}.subspan(3, 4));
        const bool result1 = (ret1 == ret2) && (ret1 == ::std::strong_ordering::equal);

        ::std::string s1 = "abcdef", s2 = "abc";
        const bool result2 = m_c(s1, s2) == ::std::strong_ordering::greater;
        const bool result3 = m_c(s2, s1) == ::std::strong_ordering::less;
        return result1 && result2 && result3;
    }

private:
    binary_comparator_interface& m_c;
};

TEST(hash, stateless)
{
    std_bin_hash h1;
    murmur_bin_hash_x64_128_xor_shift_to_64 h2;
    ASSERT_TRUE(hash_test_executor(h1).is_state_less());
    ASSERT_TRUE(hash_test_executor(h2).is_state_less());
}

TEST(comp, basic)
{
    memcmp_comparator c;
    ASSERT_TRUE(comp_test_executor(c).is_state_less());
}
