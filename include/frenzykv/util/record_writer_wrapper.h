#ifndef FRENZYKV_RECORD_WRITER_WRAPPER_H
#define FRENZYKV_RECORD_WRITER_WRAPPER_H

#include <memory>
#include "frenzykv/concepts.h"
#include "frenzykv/write_batch.h"
#include "koios/task.h"

namespace frenzykv
{

class record_writer_wrapper
{
public:
    template<is_record_writer Writer>
    record_writer_wrapper(Writer&& w)
        : m_storage{ new ::std::byte[sizeof(Writer)] }
    {
        new (m_storage.get()) Writer(::std::forward<Writer>(w));
        m_write_p = +[](::std::byte* obj, const write_batch& batch) {
            return reinterpret_cast<Writer*>(obj)->write(batch);
        };
        m_dtor_p = +[](::std::byte* obj) noexcept {
            reinterpret_cast<Writer*>(obj)->~Writer();
        };
    }

    record_writer_wrapper(record_writer_wrapper&& other) noexcept
        : m_storage{ ::std::move(other.m_storage) }, 
          m_write_p{ other.m_write_p }, 
          m_dtor_p{ other.m_dtor_p }
    {
    }

    record_writer_wrapper& operator=(record_writer_wrapper&& other) noexcept
    {
        clear();
        m_storage = ::std::move(other.m_storage);
        m_write_p = other.m_write_p;
        m_dtor_p =  other.m_dtor_p;

        return *this;
    }

    record_writer_wrapper& operator=(is_record_writer auto&& writer)
    {
        return *this = record_writer_wrapper{ ::std::forward<decltype(writer)>(write) };
    }

    ~record_writer_wrapper() noexcept { clear(); }

    auto write(const write_batch& batch)
    {
        assert(m_storage);
        return m_write_p(m_storage.get(), batch);
    }

private:
    void clear() noexcept
    {
        if (m_storage) m_dtor_p(m_storage.get());
    }

private:
    ::std::unique_ptr<::std::byte[]> m_storage;
    koios::task<> (*m_write_p)(::std::byte*, const write_batch&);
    void (*m_dtor_p)(::std::byte*);
};


} // namespace frenzykv

#endif
