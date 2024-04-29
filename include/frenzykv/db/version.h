#ifndef FRENZYKV_DB_VERSION_H
#define FRENZYKV_DB_VERSION_H

#include <memory>
#include <ranges>
#include <vector>

#include "toolpex/ref_count.h"
#include "koios/task.h"

#include "frenzykv/types.h"

namespace frenzykv
{

class version_delta
{
public:
    decltype(auto) add_compacted_files(::std::ranges::range auto const& ids)
    {
        m_compacted.append_range(ids);
        return *this;
    }

    decltype(auto) add_new_files(::std::ranges::range auto const& ids)
    {
        m_added.append_range(ids);
        return *this;
    }

    decltype(auto) add_new_file(file_id_t id)
    {
        m_added.emplace_back(id);
        return *this;
    }

    const auto& comapcted_files() const noexcept { return m_compacted; }
    const auto& added_files() const noexcept { return m_added; }

private:
    ::std::vector<file_id_t> m_compacted;
    ::std::vector<file_id_t> m_added;
};

class version
{
public:
    using sptr = ::std::shared_ptr<version>;
    
public:
    sequence_number_t sequence_number() const noexcept { return m_seq; }
    ~version() noexcept;

    bool    operator<(const version& other) const noexcept;
    version operator+(const version_delta& delta) const noexcept;

private:
    sequence_number_t m_seq;
};

class version_center
{
public:
    version_center(const kvdb_deps& deps, sequence_number_t current)
        : m_deps{ &deps }, m_current{ current };
    {
    }
    
private:
    const kvdb_deps* m_deps;
    sequence_number_t m_current;
};

} // namespace frenzykv

#endif
