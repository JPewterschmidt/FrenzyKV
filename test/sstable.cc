#include "gtest/gtest.h"

#include <memory>
#include <ranges>
#include <string>
#include <string_view>

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/util/comp.h"

#include "frenzykv/persistent/block.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

using namespace frenzykv;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;
namespace rv = ::std::ranges::views;

namespace
{

const ::std::string user_value = "WilsonAlinaWilsonAlinaWilsonAlinaWilsonAlina";
const ::std::string tomb_stone_key = "bbbdddccc";

::std::vector<kv_entry> make_kvs()
{
    ::std::vector<kv_entry> kvs{};
    ::std::string key = "aaabbbccc";

    kvs.emplace_back(0, tomb_stone_key);
    for (size_t i{}; i < 1000; ++i)
    {
        if (i % 20 == 0)
        {
            auto newkview = key | rv::transform([](auto&& ch){ return ch + 1; });
            key = ::std::string{ begin(newkview), end(newkview) };
        }

        kvs.emplace_back((sequence_number_t)i, key, user_value);
    }
    ::std::sort(kvs.begin(), kvs.end());
    return kvs;
}
    
class sstable_test : public ::testing::Test
{
public:
    void reset()
    {
        m_filter = make_bloom_filter(64);
        auto file = ::std::make_unique<in_mem_rw>(4096);
        m_file_storage = &file->storage();
        m_builder = ::std::make_unique<sstable_builder>(
            m_deps, 4096 * 1024 * 100 /*400MB*/, m_filter.get(), ::std::move(file)
        );
        m_table = nullptr;
    }

    koios::task<bool> build(const auto& kvs)
    {
        m_built = true;
        for (const auto& kv : kvs)
        {
            if (!co_await m_builder->add(kv))
                co_return false;
        }
        co_return co_await m_builder->finish();
    }

    koios::task<bool> make_table()
    {
        if (!m_built)
        {
            if (!co_await build(make_kvs()))
                co_return false;
        }

        auto file = ::std::make_unique<in_mem_rw>();
        file->clone_from(::std::move(*m_file_storage), 4096);
        m_file_storage = &file->storage();

        m_table = ::std::make_unique<sstable>(
            m_deps, m_filter.get(), ::std::move(file)
        );
        
        co_return true;
    }

    koios::task<bool> get(sequenced_key user_key)
    {
        if (!m_table) co_return false;
        auto opt = co_await m_table->get_segment(user_key);
        if (!opt) co_return false;

        auto user_key_rep = user_key.serialize_user_key_as_string();
        auto user_key_rep_b = ::std::as_bytes(::std::span{ user_key_rep });
        if (!opt->first.fit_public_prefix(user_key_rep_b)) 
            co_return false;

        for (auto entry : entries_from_block_segment(opt->first))
        {
            if (entry.key().user_key() != user_key.user_key())
                co_return false;
            if (!entry.is_tomb_stone() && entry.value().value() != user_value)
                co_return false;
        }
               
        co_return true;      
    }

    bool last_uk_exists() const
    {
        sequenced_key key = m_table->last_user_key_without_seq();
        return key.user_key().size() == "aaabbbccc"sv.size();
    }

private:
    kvdb_deps m_deps{};
    ::std::vector<buffer<>>* m_file_storage{};
    ::std::unique_ptr<sstable_builder> m_builder;
    ::std::unique_ptr<sstable> m_table;
    ::std::unique_ptr<filter_policy> m_filter;
    bool m_built{};
};

} // annoymous namespace 

TEST_F(sstable_test, build)
{
    reset();
    auto kvs = make_kvs();   
    ASSERT_TRUE(build(kvs).result());
}

TEST_F(sstable_test, get)
{
    reset();
    ASSERT_TRUE(make_table().result());
    ASSERT_TRUE(get({0, "dddeeefff"}).result());
    ASSERT_TRUE(get({0, "ggghhhiii"}).result());
    ASSERT_TRUE(last_uk_exists());
}

TEST_F(sstable_test, find_tomb_stone)
{
    reset();
    ASSERT_TRUE(make_table().result());
    ASSERT_TRUE(get({0, tomb_stone_key}).result());
}
