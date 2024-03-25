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
};

class nop_compresser : final public compresser
{
public:
    ::std::string compress(::std::string str) noexcept toverride
    {
        return str;
    }
};

} // namespace frenzykv

#endif
