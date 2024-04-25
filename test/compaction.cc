#include <memory>
#include <ranges>

#include "gtest/gtest.h"

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/persistent/compaction.h"
#include "frenzykv/db/filter.h"

namespace
{

using namespace frenzykv;
namespace rv = ::std::ranges::views;

class compaction_test : public ::testing::Test
{
public:
    void reset()
    {
    }
    
    koios::task<::std::unique_ptr<in_mem_rw>>
    make_sstable(size_t keybeg, size_t keyend)
    {
        auto result = ::std::make_unique<in_mem_rw>();
        sstable_builder builder(m_deps, 4096 * 10, m_filter.get(), result.get());
        for (auto key : rv::iota(keybeg, keyend) 
            | rv::transform([](auto&& key){ return ::std::to_string(key); })
            | rv::transform([](auto&& str){ 
                  return kv_entry(0, str, "xxxxxx");
              }))
        {
            [[maybe_unused]] bool addret = co_await builder.add(key);
            assert(addret);
        }
        co_await builder.finish();

        co_return result;       
    }

    koios::task<bool> test_merging_two()
    {
        ::std::vector fvec{ 
            co_await make_sstable(0, 1000), 
            co_await make_sstable(500, 1000),
        };
        
        compactor c(m_deps, 81920, *m_filter);
        ::std::unique_ptr<seq_writable> newfile = co_await c.merge_tables(
            fvec | rv::transform([this](auto&& f){ 
                return sstable(m_deps, m_filter.get(), f.get()); 
            }
        ));

        //const uintmax_t old_files_size = ::std::ranges::fold_left(
        //    fvec | rv::transform([](auto&& f){ 
        //        return f->file_size(); }
        //    ), 0, ::std::plus<uintmax_t>{});
        
        uintmax_t old_files_size{};
        for (const auto& filep : fvec)
        {
            old_files_size += filep->file_size();
        }

        co_return newfile->file_size() < old_files_size;
    }

private:
    kvdb_deps m_deps;
    ::std::unique_ptr<filter_policy> m_filter = make_bloom_filter(64);
};

} // annoymous namespace

TEST_F(compaction_test, basic)
{
    ASSERT_TRUE(test_merging_two().result()); 
}
