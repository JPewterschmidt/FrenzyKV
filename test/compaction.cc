#include <memory>
#include <ranges>
#include <vector>
#include <list>

#include "gtest/gtest.h"

#include "koios/task.h"

#include "frenzykv/io/in_mem_rw.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/persistent/compaction.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace
{

using namespace frenzykv;
namespace rv = ::std::ranges::views;

class compaction_test : public ::testing::Test
{
public:
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

    [[maybe_unused]]
    koios::task<bool> test_merging_two()
    {
        // This trigger a compiler bug has been reported at
        // https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114850
        //
        //[[maybe_unused]]
        //::std::vector fvec{ 
        //    co_await make_sstable(0, 1000), 
        //    co_await make_sstable(500, 1000),
        //};
        
        ::std::vector<::std::unique_ptr<in_mem_rw>> fvec;
        fvec.push_back(co_await make_sstable(0, 1000));
        fvec.push_back(co_await make_sstable(500, 1500));

        compactor c(m_deps, m_filter.get());
        auto tables_async_view = fvec | rv::transform([this](auto&& f){ 
            return sstable::make(m_deps, m_filter.get(), f.get()); 
        });

        ::std::vector<::std::shared_ptr<sstable>> tables;
        for (auto&& ta : tables_async_view)
        {
            tables.push_back(co_await ta);   
        }

        ::std::unique_ptr<in_mem_rw> newfile = co_await c.merge_tables(tables, 3);
        auto final_table = co_await sstable::make(m_deps, m_filter.get(), newfile.get());
        const auto entries = co_await get_entries_from_sstable(*final_table);
        
        co_return newfile->file_size() < ::std::ranges::fold_left(
            fvec | rv::transform([](auto&& f){ 
                return f->file_size(); }
            ), 0, ::std::plus<uintmax_t>{}) 
            && ::std::is_sorted(entries.begin(), entries.end());
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
