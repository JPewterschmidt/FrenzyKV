#ifndef FRENZYKV_TABLE_MEMTABLE_CONCEPTS_H
#define FRENZYKV_TABLE_MEMTABLE_CONCEPTS_H

#include <cstddef>
#include <system_error>

#include "koios/task_concepts.h"

#include "frenzykv/write_batch.h"
#include "frenzykv/kv_entry.h"

namespace frenzykv
{

template<typename M>
concept is_memtable = requires(M m)
{
    { m.insert(::std::declval<write_batch>())   } -> koios::task_callable_with_result_concept<::std::error_code>>;
    { m.get(::std::declval<sequenced_key>())    } -> koios::task_callable_with_result_concept<::std::optional<kv_entry>>;
    { m.count()                                 } -> koios::task_callable_with_result_concept<size_t>;
    { m.full()                                  } -> koios::task_callable_with_result_concept<bool>;
    { m.bound_size_bytes()                      } -> koios::task_callable_with_result_concept<size_t>;
    { m.size_bytes()                            } -> koios::task_callable_with_result_concept<size_t>;
    { m.could_fit_in(::std::declval<write_batch>()) } -> koios::task_callable_with_result_concept<bool>;
    { m.empty() } -> koios::task_callable_with_result_concept<bool>;
};

} // namespace frenzykv

#endif
