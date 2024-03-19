#ifndef FRENZYKV_MEMTABLE_H
#define FRENZYKV_MEMTABLE_H

#include "toolpex/skip_list.h"
#include "frenzykv/write_batch.h"
#include "koios/coroutine_mutex.h"
#include "entry_pbrep.pb.h"

namespace frenzykv
{

class memtable
{
public:
    koios::task<> insert(const write_batch& b);
    koios::task<> insert(write_batch&& b);
    koios::task<> insert(entry_pbrep entry);
    koios::task<entry_pbrep> get(const ::std::string& key) const noexcept;

private:
    koios::task<> insert_impl(entry_pbrep entry);
    
private:
    toolpex::skip_list<::std::string, ::std::string> m_list{16};
    mutable koios::mutex m_list_mutex;
};

} // namespace frenzykv

#endif
