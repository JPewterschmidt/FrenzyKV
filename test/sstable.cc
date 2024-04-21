#include "gtest/gtest.h"

#include <memory>
#include <ranges>
#include <string>
#include <string_view>

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/filter.h"

#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

using namespace frenzykv;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;
namespace rv = ::std::ranges::views;

namespace
{

::std::vector<kv_entry> make_kvs()
{
    ::std::vector<kv_entry> kvs{};
    ::std::string key = "aaabbbccc";

    for (size_t i{}; i < 10000; ++i)
    {
        if (i % 200 == 0)
        {
            auto newkview = key | rv::transform([](auto&& ch){ return ch + 1; });
            key = ::std::string{ begin(newkview), end(newkview) };
        }

        kvs.emplace_back(
            (sequence_number_t)i, 
            key, "WilsonAlinaWilsonAlinaWilsonAlinaWilsonAlina"
        );
    }
    return kvs;
}
    
class sstable_test : public ::testing::Test
{
public:
    void reset()
    {
        auto file = ::std::make_unique<in_mem_rw>(4096);
        m_file_storage = &file->storage();
        m_builder = ::std::make_unique<sstable_builder>(
            m_deps, make_bloom_filter(64), ::std::move(file)
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
            m_deps, make_bloom_filter(64), ::std::move(file)
        );
        
        co_return true;
    }

    koios::task<bool> get(const_bspan user_key)
    {
        if (!m_table) co_return false;
        auto opt = co_await m_table->get(user_key);
        if (!opt) co_return false;

        co_return opt->fit_public_prefix(user_key);
    }

private:
    kvdb_deps m_deps{};
    ::std::vector<buffer<>>* m_file_storage{};
    ::std::unique_ptr<sstable_builder> m_builder;
    ::std::unique_ptr<sstable> m_table;
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
    ::std::span cb{ "dddeeefff"sv };
    ASSERT_TRUE(get(::std::as_bytes(cb)).result());
}
