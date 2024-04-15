#ifndef FRENZYKV_CONCEPTS_H
#define FRENZYKV_CONCEPTS_H

#include <utility>
#include "frenzykv/write_batch.h"
#include "koios/task_concepts.h"

namespace frenzykv
{

template<typename T>
concept is_table = requires (T s)
{
    { s.get(::std::declval<sequenced_key>()) } 
        -> koios::task_callable_with_result_concept<
               ::std::optional<kv_entry>>;
};

template<typename T>
concept is_mutable_table = is_table<T> && requires (T t)
{
    { t.insert(::std::declval<write_batch>()) }
        -> koios::task_callable_with_result_concept<
               ::std::error_code>;
};

template<typename Writer>
concept is_record_writer = requires (Writer w)
{
    { w.write(::std::declval<write_batch>()) } -> koios::awaitible_concept;
};

} // namespace frenzykv

#endif
