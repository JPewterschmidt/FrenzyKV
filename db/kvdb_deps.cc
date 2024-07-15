// This file is part of Koios
// https://github.com/JPewterschmidt/FrenzyKV
//
// Copyleft 2023 - 2024, ShiXin Wang. All wrongs reserved.

#include "frenzykv/kvdb_deps.h"
#include "koios/exceptions.h"

namespace frenzykv
{

kvdb_deps::kvdb_deps()
    : kvdb_deps(get_global_options())
{
}

kvdb_deps::kvdb_deps(options opt, ::std::move_only_function<void()> after_init)
    : m_opt{ ::std::make_shared<options>(::std::move(opt)) }, 
      m_env{ ::std::shared_ptr(env::make_default_env(*(this->opt()))) }, 
      m_stat{ ::std::make_shared<statistics>() }
{
    namespace fs = ::std::filesystem;
    assert(m_opt.load() && m_env.load() && m_stat.load());
    ::std::error_code ec{};
    const auto rp = this->opt()->root_path;
    if (!fs::exists(rp, ec)) fs::create_directory(rp);
    if (ec) throw koios::exception{ec};

    recreate_dirs_if_non_exists();
    
    if (after_init) after_init();
}

} // namespace frenzykv
