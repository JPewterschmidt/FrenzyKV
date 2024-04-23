#ifndef FRENZYKV_READ_WRITE_OPTIONS_H
#define FRENZYKV_READ_WRITE_OPTIONS_H

namespace frenzykv
{

struct write_options
{
    bool sync_write = false;
};

struct read_options
{
    sequence_number_t sequence_number; // TODO: going to be replaced with snapshot object
};

} // namespace frenzykv

#endif
