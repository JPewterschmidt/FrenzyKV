#include <string>

#include "toolpex/exceptions.h"
#include "frenzykv/persistent/sstable.h"
#include "frenzykv/persistent/sstable_builder.h"

namespace frenzykv
{

koios::task<bool> sstable::parse_meta_data()
{
    const uintmax_t filesz = m_file->file_size();
    ::std::string buffer(sizeof(mbo_t) + sizeof(mgn_t), 0);
    
    co_await m_file->read({ buffer.data(), buffer.size() }, filesz);
    mbo_t mbo{};
    mgn_t magic_num{};
    ::std::memcpy(&mbo, buffer.data(), sizeof(mbo));
    ::std::memcpy(&magic_num, buffer.data(), sizeof(magic_num));

    // file integrity check
    if (magic_number_value() != magic_num)
    {
        co_return false;
    }

    // For meta Block
    buffer = ::std::string(filesz - mbo - sizeof(mbo_t) - sizeof(mgn_t), 0);
    co_await m_file->read({ buffer.data(), buffer.size() }, mbo);
    block meta_block{ ::std::as_bytes(::std::span{buffer}) };
    assert(meta_block.special_segments_count() == 1);
    for (block_segment seg : meta_block.segments_in_single_interval())
    {
        if (as_string_view(seg.public_prefix()) == "bloom_filter")
        {
            auto fake_user_value_sp = seg.items().front();
            auto uv = kv_user_value::parse(fake_user_value_sp);
            m_filter_rep = uv.value();
            break; // Until now, there should be only one segment, only one element in segment.
        }
    }

    co_return true;
}

} // namespace frenzykv
