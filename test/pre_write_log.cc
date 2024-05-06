#include <string>

#include "gtest/gtest.h"

#include "toolpex/errret_thrower.h"

#include "koios/iouring_awaitables.h"

#include "frenzykv/env.h"
#include "frenzykv/write_batch.h"

#include "frenzykv/log/logger.h"
#include "frenzykv/io/inner_buffer.h"

using namespace frenzykv;
using namespace ::std::string_literals;
using namespace ::std::string_view_literals;

namespace
{

write_batch make_batch()
{
    write_batch result;
    result.write("xxxx", "abc");
    result.write("aaaa", "def");
    result.set_first_sequence_num(0);

    return result;
}

koios::task<bool> write(logger& l)
{
    auto w = make_batch();
    try
    {
        co_await l.insert(w);
    }
    catch (...)
    {
        co_return false;
    }
    co_return true;
}

koios::eager_task<bool> read(env* e)
{
    auto file = e->get_seq_readable(prewrite_log_path()/prewrite_log_name());
    buffer<> buf(file->file_size());
    const uintmax_t readed = co_await file->read(buf.writable_span());
    assert(readed == file->file_size());
    buf.commit(readed);

    size_t first_entey_sz = serialized_entry_size(buf.valid_span().data());

    kv_entry entry1{ buf.valid_span() };
    bool result = (entry1.key().user_key() == "xxxx"sv);

    kv_entry entry2{ buf.valid_span().subspan(first_entey_sz) };
    result &= (entry2.key().user_key() == "aaaa"sv);

    co_return result;
}

} // annoymous namespace

TEST(pre_write_log, basic)
{
    kvdb_deps deps;
    logger l(deps);
    ASSERT_TRUE(write(l).result());
    auto e = deps.env();
    ASSERT_TRUE(read(e.get()).result());

    l.delete_file().result();
}
