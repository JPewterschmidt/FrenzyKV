#ifndef FRENZYKV_VERSION_H
#define FRENZYKV_VERSION_H

#include <memory>
#include <chrono>
#include "frenzykv/types.h"
#include "frenzykv/kvdb_deps.h"
#include "toolpex/move_only.h"

namespace frenzykv
{

class abstract_versions
{
public:
    virtual koios::task<sequence_number_t> 
    set_last_sequence_number(sequence_number_t newver) = 0;

    virtual koios::task<sequence_number_t> 
    last_sequence_number() const = 0;
};

namespace version_detials{ 

::std::unique_ptr<abstract_versions> 
default_instance(::std::string filename = "");

} // namespace version_detials

class version_hub : public toolpex::move_only
{
public:
    version_hub(const kvdb_deps& deps);

    koios::task<>                   set_last_sequence_number(sequence_number_t newver);
    koios::task<sequence_number_t>  last_sequence_number() const;

private:
    const kvdb_deps* m_deps{};
    ::std::unique_ptr<abstract_versions> m_pimpl = version_detials::default_instance();
};

} // namespace frenzykv

#endif
