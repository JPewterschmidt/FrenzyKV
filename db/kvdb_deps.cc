#include "frenzykv/kvdb_deps.h"

namespace frenzykv
{

kvdb_deps::kvdb_deps()
    : m_opt{ ::std::make_shared<options>() }, 
      m_env{ env::make_default_env(*option()) }, 
      m_stat{ ::std::make_shared<statistics>() }
{
    assert(m_opt.load() && m_env.load() && m_stat.load());
}

} // namespace frenzykv
