#ifndef FRENZYKV_WRITE_BATCH_H
#define FRENZYKV_WRITE_BATCH_H

#include <deque>
#include <string>
#include <string_view>
#include "frenzykv/frenzykv.h"
#include "frenzykv/db/kv_entry.h"

namespace frenzykv
{
    class write_batch
    {
    public:
        constexpr write_batch() noexcept = default;
        write_batch(const_bspan key, const_bspan value) { write(key, value); }
        void    write(const_bspan key, const_bspan value);
        void    write(::std::string_view key, ::std::string_view value); // Basically for debugging
        void    write(write_batch other);
        size_t  serialized_size() const;
        size_t  count() const noexcept { return m_entries.size(); }
        bool    empty() const noexcept { return m_entries.empty(); }

        /*! \brief  Write a tomb record into db.
         *  If there's any write operation relates to 
         *  the same key specified by the parameter `key` will be removed.
         */
        void remove_from_db(const_bspan key);
        void remove_from_db(::std::string_view key);

        /*! \brief  Do the compactation in this write bacth container.
         *  Usually be called in function non-const version `serialize_to()`
         */
        constexpr void compact() noexcept {}

        /*! \brief  Serialize all the kv pair into bytes form. */
        size_t  serialize_to(bspan buffer) const;
        auto begin()       noexcept { return m_entries.begin(); }
        auto end()         noexcept { return m_entries.end();   }
        auto begin() const noexcept { return m_entries.begin(); }
        auto end()   const noexcept { return m_entries.end();   }

        ::std::string to_string_debug() const;
        ::std::string to_string_log() const;

        /*! \brief Set the sequence number of the first operation of this batch.
         *  \param num the sequence number.
         *  You SHOULD set the sequence number before serialization.
         */
        void set_first_sequence_num(sequence_number_t num);

    private:
        // TODO Friend to some internal class.

        // Every single writing operation should have a sequence number, 
        // at least to the higher layers. 
        // Though, those higher layers should can get the sequence number range, 
        // Because this `write_batch` batch those writing operations into a single atomic write.
        // It's not that important to the MVCC or other components 
        // the specific seq number of each writing in a `write_batch`

        sequence_number_t first_sequence_num() const noexcept { return m_seqnumber; }
        sequence_number_t last_sequence_num() const noexcept { return static_cast<sequence_number_t>(m_seqnumber + count() - 1); }
        void repropogate_sequence_num();

    private:
        auto stl_style_remove(const_bspan key);
        
    private:
        sequence_number_t m_seqnumber{};
        ::std::vector<kv_entry> m_entries;
    };
} // namespace frenzykv

#endif
