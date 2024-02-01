#ifndef FRENZYKV_WRITE_BATCH_H
#define FRENZYKV_WRITE_BATCH_H

#include <deque>
#include "entry_pbrep.pb.h"
#include "frenzykv/frenzykv.h"

namespace frenzykv
{
    class write_batch
    {
    public:
        constexpr write_batch() noexcept = default;
        write_batch(const_bspan key, const_bspan value) { write(key, value); }
        void    write(const_bspan key, const_bspan value);
        void    write(write_batch other);
        void    remove_from_db(const_bspan key);
        size_t  serialized_size() const;
        size_t  count() const noexcept { return m_entries.size(); }
        bool    empty() const noexcept { return m_entries.empty(); }
        constexpr void compact() noexcept {}
        size_t  serialize_to_array(bspan buffer) const;
        size_t  serialize_to_array(bspan buffer);

    private:
        auto stl_style_remove(const_bspan key);
        
    private:
        ::std::vector<entry_pbrep> m_entries;
    };
} // namespace frenzykv

#endif
