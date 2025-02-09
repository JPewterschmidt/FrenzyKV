// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#ifndef FRENZYKV_TABLE_SSTABLE_BUILDER_H
#define FRENZYKV_TABLE_SSTABLE_BUILDER_H

#include <string>
#include <memory>
#include <ranges>
#include <algorithm>

#include "toolpex/assert.h"

#include "frenzykv/kvdb_deps.h"
#include "frenzykv/db/filter.h"
#include "frenzykv/persistent/block.h"

#include "frenzykv/io/writable.h"

namespace frenzykv
{

/*
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      | Data Block        |
 *      |-------------------|
 *      |         ...       |
 *      |-------------------|
 *      | Meta Block        |
 *      |-------------------|
 *      |  MBO              |   uint64_t    8B
 *      |-------------------|
 *      |  Magic Number     |   uint32_t    4B 
 *      |-------------------|
 *      
 *      MBO             Meta Block Offset   the offset position of Meta block
 *      Meta block      meta_builder.add({0, "bloomfilter"}, m_filter_rep);
 *      Magic Number    See the sstable.cc source file.
 */

using mbo_t = uint64_t;
using mgn_t = uint32_t; // magic number type

mgn_t magic_number_value() noexcept;

class sstable_builder
{
public:
    sstable_builder(const kvdb_deps& deps, 
                    uintmax_t size_limit,
                    filter_policy* filter, 
                    seq_writable* file);

    sstable_builder(const kvdb_deps& deps, 
                    uintmax_t size_limit,
                    filter_policy* filter, 
                    ::std::unique_ptr<seq_writable> file);

    ~sstable_builder() noexcept { toolpex_assert(m_finish); }

    sstable_builder(sstable_builder&& other) noexcept;
    sstable_builder& operator=(sstable_builder&& other) noexcept;
    
    koios::task<bool> add(const sequenced_key& key, const kv_user_value& value);
    koios::task<bool> add(const kv_entry& kv) { return add(kv.key(), kv.value()); }

    koios::task<bool> add(::std::ranges::range auto const& entries)
    {
        bool result{true};
        toolpex_assert(::std::is_sorted(entries.begin(), entries.end()));
        for (const auto& kv : entries)
        {
            result = co_await add(kv);
            if (!result) break;
        }
        co_return result;
    }

    bool was_finish() const noexcept { return m_finish; }
    koios::task<bool> finish();
    bool reach_the_size_limit() const noexcept { return m_size_flushed >= m_size_limit; }
    uintmax_t size_limit() const noexcept { return m_size_limit; }
    bool empty() const noexcept;

private:
    koios::task<bool> flush_current_block(bool need_flush = true);
    void swap(sstable_builder&& other);

private:
    const kvdb_deps* m_deps;
    bool m_finish{};
    uintmax_t m_size_limit{};
    uintmax_t m_size_flushed{};
    filter_policy* m_filter{};
    ::std::string m_first_uk{};
    ::std::string m_last_uk{};
    ::std::string m_filter_rep{};
    block_builder m_block_builder;

    seq_writable* m_file;
    ::std::unique_ptr<seq_writable> m_self_managed_file;
    mbo_t m_bytes_appended_to_file{};
};

} // namespace frenzykv

#endif
