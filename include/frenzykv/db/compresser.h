#ifndef FERNZYKV_DB_COMPRESSER_H
#define FERNZYKV_DB_COMPRESSER_H

#include <string>

namespace frenzykv
{
    
class compresser
{
public:
    virtual ~compresser() noexcept {}
    virtual ::std::string compress(::std::string str) = 0;
    auto operator()(auto&& str) { return compress(::std::forward<decltype(str)>(str)); }
};

class nop_compresser : final public compresser
{
public:
    ::std::string compress(::std::string str) noexcept override
    {
        return str;
    }
};

} // namespace frenzykv

#endif
