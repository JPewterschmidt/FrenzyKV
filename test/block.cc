#include "gtest/gtest.h"

#include <string>
#include <ranges>

#include "frenzykv/persistent/block.h"
#include "frenzykv/db/kv_entry.h"

using namespace frenzykv;
namespace rv = ::std::ranges::views;

TEST(block, builder)
{
    ::std::vector<kv_entry> kvs{};
    ::std::string key = "aaabbbccc";

    for (size_t i{}; i < 100000; ++i)
    {
        if (i % 2000 == 0)
        {
            auto newkview = key | rv::transform([](auto&& ch){ return ch + 1; });
            key = ::std::string{ begin(newkview), end(newkview) };
        }

        kvs.emplace_back(
            (sequence_number_t)i, 
            key, "aaaaaaaaabbbbbbbbbbbbbcccccccccccdddddd"
        );
    }
    
    kvdb_deps deps{};
    auto opt = *deps.opt();
    opt.max_block_segments_number = 10;
    deps.set_opt(::std::move(opt));
    block_builder bb{deps};
    for (const auto& item : kvs)
    {
        bb.add(item);
    }

    ::std::string block_rep = bb.finish();

    ASSERT_FALSE(block_rep.empty());
    

    // -------------- deserialization --------------------
    
    const size_t desire_ssc = ((kvs.size() / 2000) / deps.opt()->max_block_segments_number) + 1;
    block b(::std::as_bytes(::std::span{block_rep}));
    ASSERT_GE(b.special_segments_count(), desire_ssc);

    ::std::vector<kv_entry> kvs2{};
    
    for (block_segment seg : b.segments())
    {
        auto uk = seg.public_prefix();
        for (const auto& item : seg.items())
        {
            sequence_number_t seq{};
            ::std::memcpy(&seq, item.data(), sizeof(seq));
            auto uv = item.subspan(sizeof(seq));
            kvs2.emplace_back(seq, uk, uv);
        }
    }

    ASSERT_EQ(kvs2.size(), kvs.size());
}
