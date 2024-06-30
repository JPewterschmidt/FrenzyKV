#ifndef FRENZYKV_PERSISTENT_DISK_FILE_H
#define FRENZYKV_PERSISTENT_DISK_FILE_H

#include <generator>
#include <optional>

#include "koios/task.h"

#include "frenzykv/db/kv_entry.h"

#include "frenzykv/persistent/block_with_storage.h"

namespace frenzykv
{

class disk_file
{
public:
    virtual ~disk_file() noexcept {}

    /*! \brief Searching specific user key from this disk_file
     *  
     *  \return A ::std::optional<block_segment> 
     *          represents a block segment that all elements 
     *          has the same public prefix (user key for data block).
     *
     *  \attention  `block_segment` only borrow bytes contents 
     *              from some RAII object that actually hold the memory.
     *              So after a call of this function, 
     *              any access to the previously returned 
     *              `block_segment` object is undefined behaviour.
     *  \param  user_key_ignore_seq Only take the serialized user key part, ignore the seq part
     */
    virtual koios::task<::std::optional<::std::pair<block_segment, block_with_storage>>> 
    get_segment(const sequenced_key& user_key_ignore_seq) const = 0;

    /*! \brief Searching specific sequenced key from this disk_file 
     *  
     *  \return A ::std::optional<kv_entry> 
     *          represents the kv_entry object that metch the sequenced key 
     *          which sequence number also involved in the searching process.
     *
     *  \param  seq_key A typical sequenced key which sequence number also 
     *          get involved in the searching process unlike the member function `get_segment()`
     */
    virtual koios::task<::std::optional<kv_entry>>
    get_kv_entry(const sequenced_key& seq_key) const = 0;

    virtual sequenced_key last_user_key_without_seq() const = 0;
    virtual sequenced_key first_user_key_without_seq() const = 0;

    virtual bool overlapped(const disk_file& other) const = 0;
    virtual bool disjoint(const disk_file& other) const = 0;
    virtual bool empty() const = 0;

    virtual bool operator<(const disk_file& other) const noexcept = 0;
    virtual bool operator==(const disk_file& other) const noexcept = 0;

    // Required by `get_segment()`
    virtual koios::task<::std::optional<block_with_storage>> 
    get_block(uintmax_t offset, btl_t btl) const = 0;

    virtual koios::task<::std::optional<block_with_storage>> 
    get_block(::std::pair<uintmax_t, btl_t> p) const
    {
        return get_block(p.first, p.second);
    }

    virtual ::std::generator<::std::pair<uintmax_t, btl_t>> block_offsets() const = 0;

    virtual size_t hash() const noexcept = 0;
    virtual ::std::string_view filename() const = 0;
};

} // namespace frenzykv

#endif
