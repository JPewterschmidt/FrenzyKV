#include "gtest/gtest.h"
#include "frenzykv/db/kv_entry.h"

using namespace frenzykv;
using namespace ::std::string_view_literals;

namespace
{

class kv_entry_format : public ::testing::Test
{
public:
    kv_entry_format() { reset(); }

    bool reset()
    {
        m_buffer = ::std::string{};
        size_t actual_len{};
        for (sequence_number_t i{}; i < 10; ++i)
        {
            auto e = kv_entry(i, "abc", "def");
            actual_len += e.serialized_bytes_size();
            e.serialize_append_to_string(m_buffer);
        }
        actual_len += append_eof_to_string(m_buffer);
        return actual_len == m_buffer.size();
    }
    
    const ::std::string& buffer() const noexcept { return m_buffer; }

    ::std::string generate_rep(auto&& entries) 
    {
        ::std::string result{};
        for (const auto& item : entries)
        {
            item.serialize_append_to_string(result);
        } 
        append_eof_to_string(result);
        
        return result;
    }

private:
    ::std::string m_buffer{};
};

} // annoymous namespace

TEST_F(kv_entry_format, init)
{
    ASSERT_TRUE(reset());
}

TEST_F(kv_entry_format, parser_generator)
{
    ASSERT_TRUE(reset());
    ASSERT_EQ(generate_rep(kv_entries_from_buffer(buffer())), buffer());
}
