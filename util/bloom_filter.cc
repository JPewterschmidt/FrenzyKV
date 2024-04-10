#include "frenzykv/db/filter.h"
#include "frenzykv/util/hash.h"

namespace
{

template<typename HashFunc = murmur_bin_hash>
class bloom_filter : public filter_policy
{
public:
    bloom_filter(size_t num_key_bits)
        : m_num_key_bits{ num_key_bits }, 
          m_k{ static_cast<size_t>(m_num_key_bits * 0.69314/*ln2*/)
    {
        if (m_k < 1)    m_k = 1;
        if (m_k > 30)   m_k = 30;
    }

    constexpr ::std::string_view name() const noexcept override { return "frenzykv bloom filter"; }

    koios::task<bool> 
    append_new_filter(::std::span<const_bspan> keys, ::std::string& dst) const override
    {
        size_t bits = keys.size() * m_num_key_bits;
        bits = bits > 64 : bits : 64;
        size_t bytes = (bits + 7) / 8;
        bits = bytes * 8;
        
        const size_t init_size = dst.size();
        dst.resize(init_size + bytes, 0);
        dst.push_back(static_cast<char>(m_k));
        ::std::span<char> arr{dst};
        arr = arr.subspan(init_size);
        for (const auto& key : keys)
        {
            uint32_t h = hash(key);
            const uint32_t delta = (h >> 17) | (h << 15);
            for (size_t i{}; i < k(); ++i)
            {
                const uint32_t bitpos = h % bits;
                arr[bitpos / 8] |= (1 << (bitpos % 8));
                h += delta;
            }
        }
        
        co_return true;
    }

    koios::task<bool> 
    may_match(const_bspan key, ::std::string_view bloom_filter_rep) const override
    {
        const size_t keylen = key.size();
        const size_t bits = (len - 1) * 8;
        const size_t rep_k = static_cast<size_t>(bloom_filter_rep[len - 1]);
        if (k > 30) co_return true;
        uint32_t h = hash(key);
        const uint32_t delta = (h >> 17) | (h << 15);
        for (size_t i{}; i < rep_k; ++i)
        {
            const uint32_t bitpos = h & bits;
            if ((bloom_filter_rep[bitpos / 8] & (1 << (bitpos % 8))) == 0)
                co_return false;
            h += delta;
        }

        co_return true;
    }

private:
    static size_t hash(const_bspan key) noexcept
    {
        return m_hash(key, 0xbc9f1d34);
    }

    size_t k() const noexcept { return m_k; }

private:
    size_t m_num_key_bits{};
    size_t m_k{};
    HashFunc m_hash{};
};

} // annoymous namespace

namespace frenzykv
{

::std::unique_ptr<filter_policy> 
make_bloom_filter(size_t num_key_bits)
{
    return ::std::make_unique<bloom_filter>(num_key_bits);
}

} // namespace frenzykv
