#ifndef FRENZYKY_DB_H
#define FRENZYKY_DB_H

#include <span>
#include "frenzykv/frenzykv.h"
#include "frenzykv/options.h"

namespace frenzykv
{
    class db_impl
    {
    public:
        koios::task<size_t> write(const_bspan key, const_bspan value, write_options opt)
        {
            return write({key, value}, opt);
        }

        koios::task<size_t> write(write_bacth batch, write_options opt);
        koios::task<size_t> remove_from_db(const_bspan key, write_options opt)
        {
            write_bacth batch;
            batch.remove_from_db(key);
            return write(::std::move(batch), opt);
        }
    };
} // namespace frenzykv

#endif
