#include "frenzykv/error_category.h"
#include <string>

namespace frenzykv
{

namespace
{
    class kvdb_category_t : public ::std::error_category
    {
    public:
        virtual const char* name() const noexcept override { return "Frenzy kvdb category"; }
        virtual ::std::string message(int condition) const override;
    };

    ::std::string 
    kvdb_category_t::
    message(int condition) const
    {
        switch (condition)
        {
            case status_t::FRZ_KVDB_OK:               return "Ok";
            case status_t::FRZ_KVDB_NOTFOUND:         return "Not Found";
            case status_t::FRZ_KVDB_CORRUPTION:       return "Corruption";
            case status_t::FRZ_KVDB_NOT_IMPLEMENTED:  return "Not Implemented";
            case status_t::FRZ_KVDB_INVALID_ARGUMENT: return "Invalid Argument";
            case status_t::FRZ_KVDB_IO_ERROR:         return "IO Error";
            default: 
            case status_t::FRZ_KVDB_UNKNOW:  return "Unknown Status";
        }
    }
} // namespace <anaoymous>

const ::std::error_category& kvdb_category() noexcept
{
    static const kvdb_category_t result;
    return result;
}

} // namespace frenzykv
