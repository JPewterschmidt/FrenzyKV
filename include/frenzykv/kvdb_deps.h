#ifndef FRENZYKV_KVDB_DEPS_H
#define FRENZYKV_KVDB_DEPS_H

#include <memory>
#include <atomic>
#include "frenzykv/options.h"
#include "frenzykv/env.h"
#include "frenzykv/statistics.h"

namespace frenzykv
{

/*! \brief the dependencies of db_impl 
 *         which needed been shared among several components.
 *
 *  \attention This object has no any inner mutex.
 *             But several deps may has.
 *             Not lock free yet.
 *
 *  \todo make it based on RCU.
 *
 *  To the one who is going to update this type:
 *  all the member (deps) should be managed by unique_ptr, 
 *  or other smart pointer or RCU faciality. 
 *  Or you have to remember to implement the move ctor/`operator=`
 *
 *  To the one use this type:
 *  This typs of objects should not be copied.
 *  Users of this type of obejects should not 
 *  constantly hold any of object reference 
 *  returned by the member functions of this type, 
 *  because any potential update could cause dangling reference.
 *
 *  \see `db_impl`
 *  \see `options`
 *  \see `env`
 *  \see `statistics`
 */
class kvdb_deps
{
public:
    friend class kvdb_deps_manipulator;
    kvdb_deps();
    kvdb_deps(options opt);

    const auto opt() const noexcept { return m_opt.load(::std::memory_order_relaxed); }
    const auto env() const noexcept { return m_env.load(::std::memory_order_relaxed); } 
    auto stat()      const noexcept { return m_stat.load(::std::memory_order_relaxed); }

private: // Deps
    ::std::atomic<::std::shared_ptr<options>>     m_opt;
    ::std::atomic<::std::shared_ptr<frenzykv::env>>         m_env;
    ::std::atomic<::std::shared_ptr<statistics>>  m_stat;
};

class kvdb_deps_manipulator
{
public:
    constexpr kvdb_deps_manipulator(kvdb_deps& deps) noexcept
        : m_deps{ deps }
    {
    }

    ::std::shared_ptr<options>  
    exchange_option(::std::shared_ptr<options> newopt)
    {
        return m_deps.m_opt.exchange(
            ::std::move(newopt), 
            ::std::memory_order_relaxed
        );
    }

    ::std::shared_ptr<env>      
    exchange_environment(::std::shared_ptr<env> newenv)
    {
        return m_deps.m_env.exchange(
            ::std::move(newenv), 
            ::std::memory_order_relaxed
        );
    }

private:
    kvdb_deps& m_deps;
};

} // namespace frenzykv

#endif
