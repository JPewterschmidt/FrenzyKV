#ifndef FRENZYKV_WRITE_BATCH_H
#define FRENZYKV_WRITE_BATCH_H

#include <deque>
#include <string_view>
#include "entry_pbrep.pb.h"
#include "frenzykv/frenzykv.h"
#include "db/version.h"

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
         *  Usually be called in function non-const version `serialize_to_array()`
         */
        constexpr void compact() noexcept {}

        /*! \brief  Serialize all the kv pair into bytes form. */
        size_t  serialize_to_array(bspan buffer) const;
        auto begin()       noexcept { return m_entries.begin(); }
        auto end()         noexcept { return m_entries.end();   }
        auto begin() const noexcept { return m_entries.begin(); }
        auto end()   const noexcept { return m_entries.end();   }

    private:
        // TODO Friend to some internal class.

        // Every single writing operation should have a sequence number, 
        // at least to the higher layers. 
        // Though, those higher layers should can get the sequence number range, 
        // Because this `write_batch` batch those writing operations into a single atomic write.
        // It's not that important to the MVCC or other components 
        // the specific seq number of each writing in a `write_batch`

        /*! \brief Set the sequence number of the first operation of this batch.
         *  \param num the sequence number.
         *  You SHOULD set the sequence number before serialization.
         */
        void set_first_sequence_num(sequence_number_t num) { m_seqnumber = num; }
        sequence_number_t first_sequence_num() const noexcept { return m_seqnumber; }
        sequence_number_t last_sequence_num() const noexcept { return m_seqnumber + count() - 1; }

    private:
        auto stl_style_remove(const_bspan key);
        
    private:
        sequence_number_t m_seqnumber{};
        ::std::vector<entry_pbrep> m_entries;
    };
} // namespace frenzykv

#endif
