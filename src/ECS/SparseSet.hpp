// SparseSet.hpp

#ifndef ECS_SPARSE_SET_HPP
#define ECS_SPARSE_SET_HPP

#include "Logging.hpp"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <type_traits>
#include <vector>

namespace ECS
{


// ISparseSet
struct ISparseSet 
{
    virtual ~ISparseSet() = default;
    virtual void Delete(size_t) = 0;
};

// SparseSet
template <typename T, typename Y>
class SparseSet : public ISparseSet
{
public:
    SparseSet(size_t r);
    SparseSet(void);

    Y *Get(T k);
    Y *Set(T k, Y v);

    void Delete(T k) override;

    const std::vector<Y> &Values() const;

private:
    static constexpr size_t MAX_ELEMENTS = 2048u;
    static constexpr size_t NULL_INDEX = std::numeric_limits<size_t>::max();

    std::vector<size_t> m_Sparse;
    std::vector<Y> m_Dense;
    std::vector<size_t> m_DenseToSparse;
    
private:
    size_t GetDenseIndex(T k) const;
};

// SparseSet Impl
template <typename T, typename Y>
SparseSet<T, Y>::SparseSet(size_t r)
{
    // T should be typecastable to size_t for indexing
    ECS_LOG_STATIC_ASSERT((std::is_convertible_v<T, size_t>));
    m_Sparse.reserve(r);
}

template <typename T, typename Y>
SparseSet<T, Y>::SparseSet(void)
    : SparseSet(64u)
{
}

template <typename T, typename Y>
Y *SparseSet<T, Y>::Get(T k)
{
    size_t index = this->GetDenseIndex(k);
    if (index == NULL_INDEX)
        return nullptr;

    return &m_Dense[index];
}

template <typename T, typename Y>
Y *SparseSet<T, Y>::Set(T k, Y v)
{
    size_t index = this->GetDenseIndex(k);
    if (index != NULL_INDEX)
    {
        m_Dense[index] = v;
        return &m_Dense[index];
    }

    size_t sk = (size_t) k;
    if (sk >= m_Sparse.size())
    {
        size_t o = m_Sparse.size();
        m_Sparse.resize(sk + 1);

        std::fill(m_Sparse.begin() + o, m_Sparse.end(), NULL_INDEX);
    }

    m_Sparse[sk] = m_Dense.size();
    
    m_Dense.push_back(v);
    m_DenseToSparse.push_back(sk);

    return &m_Dense.back();
}

template <typename T, typename Y>
void SparseSet<T, Y>::Delete(T k)
{
    size_t index = this->GetDenseIndex(k);
    if (index == NULL_INDEX)
        return;

    size_t sk = (size_t) k;

    std::swap(m_Sparse[m_DenseToSparse.back()], m_Sparse[sk]);
    std::swap(m_Dense.back(), m_Dense[index]);
    std::swap(m_DenseToSparse.back(), m_DenseToSparse[index]);

    m_Dense.pop_back();
    m_DenseToSparse.pop_back();

    m_Sparse[sk] = NULL_INDEX;
}

template <typename T, typename Y>
const std::vector<Y> &SparseSet<T, Y>::Values() const
{
    return this->m_Dense;
}

template <typename T, typename Y>
size_t SparseSet<T, Y>::GetDenseIndex(T k) const
{
    size_t sk = (size_t) k;
    ECS_LOG_ASSERT(sk < MAX_ELEMENTS);

    if (sk >= m_Sparse.size())
        return NULL_INDEX;

    return m_Sparse[sk];
}


}

#endif
