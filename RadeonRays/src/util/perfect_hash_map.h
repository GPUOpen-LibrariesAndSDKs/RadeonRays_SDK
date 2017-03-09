#pragma once
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <array>
#include <cstdlib>
#include <map>

// Round up to next power of two
template <typename T> inline T round_up_to_pow2(T v);

template <> inline int round_up_to_pow2<int>(int v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

template <> inline unsigned round_up_to_pow2<unsigned>(unsigned v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

// Establish perfect mapping between specified keys and values
// O(1) worst case lookup time
template <typename K, typename V, typename D = int> class PerfectHashMap
{
public:
    // max_key is the upper bound for all the keys in keys array
    // keys and values are arrays of size count
    // invalid_value is returned by the query later if there is no such key in the table
    PerfectHashMap(K max_key, K const* keys, V const* values, D count, V invalid_value);
    PerfectHashMap() = delete;

    // Look up value for a specified key
    // O(1) time
    V operator[](K key) const;


    // DEBUG:
    D hash_table_size() const { return static_cast<D>(m_hash_table.size()); }
    D displacement_table_size() const { return m_t; }
    D const* displacement_table_ptr() const { return &m_displacement[0]; }
    V const* hash_table_ptr() const { return &m_hash_table[0]; }

private:
    // Intermediate table dim, should be at least sqrt(max_key)
    D m_t;
    // Displacement table of rows
    std::vector<D> m_displacement;
    // Hash table
    std::vector<V> m_hash_table;
};


template <typename K, typename V, typename D>
inline
PerfectHashMap<K,V,D>::PerfectHashMap(K max_key, K const* keys, V const* values, D count, V invalid_value)
{
    // 1. We first hash keys into an intermediate 2D array of m_t * m_t size
    // 2. We build a histogram over rows
    // 3. We sort rows according to this histogram
    // 4. We iterate over all the rows and shift each one to the right
    //    until it does not have collisions with previous rows (each column has
    //    at most 1 element), storing offsets into m_displacement array.
    // 5. We compress the rows into a hash table.

    // Row structure to build histogram over rows
    struct Row
    {
        D index;
        D count;
    };

    // Row vector
    std::vector<Row> rows;
    // Intermediate 2D array to hash keys into
    std::vector<V> intermediate;

    // Determine intermediate table size:
    // to be able to hash max_key it should be at least sqrt(max_key)
    m_t = round_up_to_pow2(static_cast<D>(std::ceil(std::sqrt(max_key))));
    // Allocate intermediate table
    intermediate.resize(m_t * m_t);
    // Allocate displacement table
    m_displacement.resize(m_t);
    // Allocate space for row histogram bins
    rows.resize(m_t);

    // Initialize rows
    for (auto i = 0; i < rows.size(); ++i)
    {
        rows[i].count = 0;
        rows[i].index = i;
    }

    // Initialize tables
    std::fill(std::begin(intermediate), std::end(intermediate), invalid_value);
    std::fill(std::begin(m_displacement), std::end(m_displacement), invalid_value);

    // Hash keys into an intermediate table
    for (auto i = 0; i < count; ++i)
    {
        auto key = keys[i];
        auto value = values[i];

        // Check max key constraint
        if (key > max_key)
            throw std::runtime_error("Max key condition violated");

        // Hash key into row, col
        auto row = key / m_t;
        // We can & (m_t - 1) since it is pow of 2
        auto col = key & (m_t - 1);

        // Update intermediate table
        intermediate[row * m_t + col] = value;

        // Update histogram
        ++rows[row].count;
    }

    // Sort rows in descending order based on histogram
    std::sort(std::begin(rows), std::end(rows), [](Row const& lhs, Row const& rhs)
    {
        return lhs.count > rhs.count;
    });

    // Start shifting rows to the right
    for (auto& row: rows)
    {
        // If we have zero here it is guaranteed we can early-out
        // since the rows are sorted in descending order.
        if (!row.count)
        {
            break;
        }

        // Try increasing offsets
        auto idx = row.index;
        for (auto offset = 0;;++offset)
        {
            bool collision = false;
            // Check if current row has no collisions
            for (auto i = 0; i < m_t; ++i)
            {
                // Check if there is current element which can potentially create collision
                if (intermediate[idx * m_t + i] != invalid_value)
                {
                    // Check if collision is actually happening
                    auto dstidx = offset + i;
                    // If dstidx > m_hash_table size there can be no collision, since
                    // no rows has been compressed into the table past the end
                    if (dstidx < m_hash_table.size() && m_hash_table[dstidx] != invalid_value)
                    {
                        collision = true;
                        break;
                    }
                }
            }

            // We checked all elements and found no collisions
            // with current offset
            if (!collision)
            {
                // Store displacement value for the row
                m_displacement[idx] = offset;

                // Compress the row into the table
                for (auto i = 0; i < m_t; ++i)
                {
                    auto value = intermediate[idx * m_t + i];
                    auto dstidx = offset + i;

                    // If we are adding past the end we do not care of
                    // missing keys (successive rows can rewrite missing values)
                    if (dstidx >= m_hash_table.size())
                    {
                        m_hash_table.push_back(value);
                    }
                    else
                    {
                        // If we are adding prior to the end we only add valid entries
                        // in order not to overwrite existing ones.
                        if (value != invalid_value)
                        {
                            m_hash_table[dstidx] = value;
                        }
                    }

                }

                // Stop trying offsets and go to the next row
                break;
            }
        }
    }
}

template <typename K, typename V, typename D>
inline
V PerfectHashMap<K,V,D>::operator[](K key) const
{
    // Hash key into row, col
    auto row = key / m_t;
    auto col = key & (m_t - 1);
    // Find displacement for the row
    auto d = m_displacement[row];
    // Return hashed value
    return m_hash_table[d + col];
}
