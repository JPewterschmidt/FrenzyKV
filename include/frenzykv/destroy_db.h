#ifndef FRENZYKV_DESTROY_DB_H
#define FRENZYKV_DESTROY_DB_H

#include <filesystem>
#include <string_view>

namespace frenzykv 
{

void destroy_db(::std::filesystem::path p, 
                ::std::string_view caller_declaration);

} // namespace frenzykv

#endif
