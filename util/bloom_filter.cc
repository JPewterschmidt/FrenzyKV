#include "frenzykv/db/filter.h"
#include "frenzykv/util/hash.h"

namespace frenzykv
{

namespace
{

auto get_delta(::std::integral auto h)
{
    return (h >> 33) | (h << 31);
}

//template<typename HashFunc = murmur_bin_hash_x64_128_xor_shift_to_64>
template<typename HashFunc = murmur_bin_hash_x64_128_xor_shift_to_64>
class bloom_filter : public filter_policy
{
public:
    bloom_filter(size_t num_key_bits)
        : m_num_key_bits{ num_key_bits }, 
          m_k{ static_cast<size_t>((double)m_num_key_bits * 0.69314/*ln2*/) }
    {
        if (m_k < 1)    m_k = 1;
        if (m_k > 60)   m_k = 60;
    }

    constexpr ::std::string_view name() const noexcept override { return "frenzykv bloom filter"; }

    bool append_new_filter(::std::span<const_bspan> keys, ::std::string& dst) const override
    {
        size_t bits = static_cast<size_t>(keys.size() * m_num_key_bits);
        bits = bits > 64 ? bits : 64;
        size_t bytes = (bits + 7) / 8;
        bits = bytes * 8;
        
        const size_t init_size = dst.size();
        dst.resize(init_size + bytes, 0);
        dst.push_back(static_cast<char>(m_k));
        ::std::span<char> arr{dst};
        arr = arr.subspan(init_size);
        for (const auto& key : keys)
        {
            size_t h = hash(key);
            const size_t delta = get_delta(h);
            for (size_t i{}; i < k(); ++i)
            {
                const size_t bitpos = h % bits;
                arr[bitpos / 8] |= static_cast<unsigned char>((1 << (bitpos % 8)));
                h += delta;
            }
        }
        
        return true;
    }

    bool may_match(const_bspan key, ::std::string_view bloom_filter_rep) const override
    {
        const size_t filter_len = bloom_filter_rep.size();
        if (filter_len < 2) return false;
        const size_t bits = (filter_len - 1) * 8;
        const size_t rep_k = static_cast<size_t>(bloom_filter_rep[filter_len - 1]);
        if (rep_k > 30) return true;
        size_t h = hash(key);
        const size_t delta = get_delta(h);
        for (size_t i{}; i < rep_k; ++i)
        {
            const size_t bitpos = h % bits;
            if ((bloom_filter_rep[bitpos / 8] & (1 << (bitpos % 8))) == 0)
                return false;
            h += delta;
        }

        return true;
    }

private:
    size_t hash(const_bspan key) const noexcept
    {
        return static_cast<size_t>(m_hash(key));
    }

    size_t k() const noexcept { return m_k; }

private:
    size_t m_num_key_bits{};
    size_t m_k{};
    HashFunc m_hash{};
};

} // annoymous namespace

::std::unique_ptr<filter_policy> 
make_bloom_filter(size_t num_key_bits)
{
    return ::std::make_unique<bloom_filter<> >(num_key_bits);
}

} // namespace frenzykv
