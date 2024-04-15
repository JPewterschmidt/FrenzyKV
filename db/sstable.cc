#include "frenzykv/db/sstable.h"
#include "toolpex/exceptions.h"

namespace frenzykv
{

class immutable_sstable : public sstable_interface
{
public:
    virtual 
    koios::task<bool> 
    write(const write_batch& batch) override
    {
        throw ::std::logic_error{"immutable_sstable::write(): this is a immutable sstable, you can not write anything to it."};       
    }

    virtual 
    koios::task<::std::optional<kv_entry>> 
    get(const sequenced_key& key) const
    {
        toolpex::not_implemented();       
        co_return {};
    }
     
protected:
    ::std::unique_ptr<random_readable> m_file;
};

class level1_sstable : public sstable_interface
{
public:
    virtual 
    koios::task<bool> 
    write(const write_batch& batch) override
    {
        toolpex::not_implemented();       
        co_return {};
    }
};

} // namespace frenzykv
