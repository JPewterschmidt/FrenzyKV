// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "gtest/gtest.h"

#include <string>
#include <string_view>
#include <ranges>

#include "frenzykv/persistent/block.h"
#include "frenzykv/db/kv_entry.h"

using namespace frenzykv;
using namespace ::std::string_view_literals;
using namespace ::std::string_literals;
namespace r = ::std::ranges;
namespace rv = r::views;

namespace
{

class block_test : public ::testing::Test
{
public:
    ::std::vector<kv_entry> make_kvs()
    {
        ::std::vector<kv_entry> kvs{};
        ::std::string key = "aaabbbccc";

        for (size_t i{}; i < 10000; ++i)
        {
            if (i % 200 == 0)
            {
                key = key | rv::transform([](auto&& ch){ return ch + 1; }) | r::to<::std::string>();
            }

            kvs.emplace_back(
                (sequence_number_t)i, 
                key, "WilsonAlinaWilsonAlinaWilsonAlinaWilsonAlina"
            );
        }
        m_kvs = kvs;
        return kvs;
    }

    void set_deps(const kvdb_deps& deps) 
    { 
        auto m = kvdb_deps_manipulator(m_deps);
        m.exchange_option(deps.opt());
        m.exchange_environment(deps.env());
    }

    bool generate_serialized_storage(const auto& kvs)
    {
        // You can serialize kv into SSTable via block_builder, 
        // but block is also a basic conponents of an sstable, 
        // the sstable also need add some metadata before and after those blocks.
        //
        // Until now(2024-04-19), the serialization and deserialization phase 
        // of block are NOT support partial process.

        block_builder bb{m_deps};
        for (const auto& item : kvs)
        {
            bb.add(item);
        }

        m_storage = bb.finish();
        return !m_storage.empty();
    }

    bool generate_compressed_storage(const auto& kvs)
    {
        auto opt = m_deps.opt();
        block_builder bb{m_deps, get_compressor(*opt, "zstd")};
        for (const auto& item : kvs)
        {
            bb.add(item);
        }

        m_storage = bb.finish();
        return !m_storage.empty();
    }

    const ::std::string& storage() const noexcept { return m_storage; }

    void reset()
    {
        m_storage = {};
        m_kvs = {};
    }

    kvdb_deps m_deps{};

    bool contents_test(const auto& kvs)
    {
        bool result{ true };

        const size_t desire_ssc = ((kvs.size() / 200) / m_deps.opt()->max_block_segments_number) + 1;
        auto bc = ::std::as_bytes(::std::span{storage()});
        result &= block_integrity_check(bc);
        if (!result) 
        {
            ::std::cerr << "block_integrity_check fail" << ::std::endl;
            return result;
        }

        ::std::string new_storage;
        if (block_content_was_comprssed(bc))
        {
            new_storage = block_decompress(bc, get_compressor(*m_deps.opt()));
            bc = ::std::as_bytes(::std::span{new_storage});
        }

        block b(bc);
        result &= (b.special_segments_count() >= desire_ssc);
        if (!result) 
        {
            return result;
        }

        ::std::vector<kv_entry> kvs2{};

        const auto& ssps = b.special_segment_ptrs();
        
        if (ssps.size() == 1)
        {
            for (block_segment seg : b.segments_in_single_interval())
            {
                for (auto entry : entries_from_block_segment(seg))
                    kvs2.push_back(::std::move(entry));
            }
        }
        else
        {
            for (auto [beg, end] : ssps | rv::adjacent<2>)
            {
                for (block_segment seg : b.segments_in_single_interval(beg, end))
                {
                    for (auto entry : entries_from_block_segment(seg))
                        kvs2.push_back(::std::move(entry));
                }
            }
        }

        result &= (kvs2 == kvs);
        return result;
    }

private:
    ::std::string m_storage{};
    ::std::vector<kv_entry> m_kvs;
};

} // annoymous namespace

TEST_F(block_test, builder)
{
    reset();

    auto kvs = make_kvs();
    kvdb_deps deps{};
    auto opt = ::std::make_shared<options>(*deps.opt());
    opt->max_block_segments_number = 100;
    opt->need_compress = false;
    kvdb_deps_manipulator(deps).exchange_option(opt);

    set_deps(deps);
    ASSERT_TRUE(generate_serialized_storage(kvs));
}

TEST_F(block_test, deserialization)
{
    reset();

    auto kvs = make_kvs();
    kvdb_deps deps{};
    auto opt = ::std::make_shared<options>(*deps.opt());
    opt->max_block_segments_number = 100;
    opt->need_compress = false;
    kvdb_deps_manipulator(deps).exchange_option(opt);

    set_deps(deps);
    generate_serialized_storage(kvs);
    
    ASSERT_TRUE(contents_test(kvs));
}

TEST_F(block_test, compression)
{
    reset();

    auto kvs = make_kvs();
    kvdb_deps deps{};
    auto opt = ::std::make_shared<options>(*deps.opt());
    opt->max_block_segments_number = 100;
    opt->need_compress = true;
    kvdb_deps_manipulator(deps).exchange_option(opt);

    set_deps(deps);
    ASSERT_TRUE(generate_compressed_storage(kvs));

    ASSERT_TRUE(contents_test(kvs));
}

TEST_F(block_test, get)
{
    reset();
    generate_serialized_storage(make_kvs());
    auto bc = ::std::as_bytes(::std::span{storage()});
    block b(bc);
    sequenced_key key{0, "dddeeefff"};
    auto key_rep = key.serialize_user_key_as_string();
    auto key_rep_b = ::std::as_bytes(::std::span{ key_rep });
    auto seg_opt = b.get(key_rep_b);
    ASSERT_TRUE(seg_opt.has_value());
    ASSERT_TRUE(seg_opt->larger_equal_than_this_public_prefix(key_rep_b));
}
