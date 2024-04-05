#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

kvdb_deps::kvdb_deps()
    : kvdb_deps(get_global_options())
{
}

kvdb_deps::kvdb_deps(options opt)
    : m_opt{ ::std::make_shared<options>(::std::move(opt)) }, 
      m_env{ ::std::shared_ptr(env::make_default_env(*(this->opt()))) }, 
      m_stat{ ::std::make_shared<statistics>() }
{
    assert(m_opt.load() && m_env.load() && m_stat.load());
}

} // namespace frenzykv
