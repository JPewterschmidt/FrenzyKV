#include "gtest/gtest.h"

#include <memory>

#include "toolpex/functional.h"
#include "koios/task.h"

#include "frenzykv/table/sstable.h"
#include "frenzykv/table/sstable_builder.h"
#include "frenzykv/write_batch.h"
#include "frenzykv/table/memtable.h"

#include "frenzykv/io/in_mem_rw.h"

namespace
{

using namespace frenzykv;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;
namespace rv = ::std::ranges::views;

class combined_components : public ::testing::Test
{
public:
    combined_components()
    {
        auto opt = ::std::make_shared<options>(get_global_options());
        opt->memory_page_bytes = 40960;
        kvdb_deps_manipulator{m_deps}.exchange_option(opt);
    }

    koios::task<memtable> make_memtable(write_batch batch)
    {
        memtable result(m_deps);
        co_await result.insert(::std::move(batch));
        co_return result;
    }

    write_batch make_write_batch(int endval = 1000)
    {
        write_batch result;
        result.set_first_sequence_num(100);

        for (int i{1}; i < endval; ++i)
        {
            result.write(::std::to_string(i), 
                         ::std::string(::toolpex::lazy_string_concater{} 
                             + "xxxxxxxxxx"sv + i));
        }

        return result;
    }

    koios::task<::std::unique_ptr<in_mem_rw>> make_sstable(memtable mem)
    {
        auto file = ::std::make_unique<in_mem_rw>();
        sstable_builder builder(m_deps, 8192 * 10, m_filter.get(), file.get());

        auto sto = co_await mem.get_storage();
        for (const auto& [k, v] : sto)
        {
            [[maybe_unused]] bool addret = co_await builder.add(k, v);
            assert(addret);
        }
        co_await builder.finish();
        co_return file;
    }
    
    koios::task<bool> sstable_test(random_readable* table, const write_batch& batch)
    {
        auto tab = co_await sstable::make(m_deps, m_filter.get(), table);
        auto entries = co_await get_entries_from_sstable(*tab);
        co_return ::std::ranges::equal(entries, batch);
    }

private:
    ::std::unique_ptr<filter_policy> m_filter = make_bloom_filter(64);
    kvdb_deps m_deps;
};

koios::task<bool> memtable_to_sstable_testbody(combined_components& c)
{
    auto batch = c.make_write_batch();
    auto mem = co_await c.make_memtable(batch);
    auto tablefilep = co_await c.make_sstable(::std::move(mem));
    co_return co_await c.sstable_test(tablefilep.get(), batch);   
}

} // annoymous namespace 

TEST_F(combined_components, memtable_to_sstable)
{
    ASSERT_TRUE(memtable_to_sstable_testbody(*this).result());
}
