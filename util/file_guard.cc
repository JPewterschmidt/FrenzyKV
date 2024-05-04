#include "frenzukv/env.h"
#include "frenzykv/util/file_gaurd.h"

namespace frenzykv
{

::std::unique_ptr<random_readable> 
file_rep::open_read(env* e) const
{
    return e->get_random_readable(sstables_path()/name());
}

::std::unique_ptr<seq_writable> 
file_rep::open_write(env* e) const
{
    return e->get_seq_writable(sstables_path()/name());
}

} // namespace frenzykv
