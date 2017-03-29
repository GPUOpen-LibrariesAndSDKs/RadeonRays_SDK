#include "collector.h"
#include "CLW/clwscene.h"
#include "SceneGraph/iterator.h"
#include <vector>
#include <cassert>

namespace Baikal
{
    using ItemMap = std::map<void const*, int>;
    using ItemSet = std::set<void const*>;
    
    class BundleImpl : public Bundle
    {
    public:
        BundleImpl(ItemMap const& map, ItemSet const& set)
        : m_map(map)
        , m_set(set)
        {
        }
        
        ItemMap m_map;
        ItemSet m_set;
    };
    
    struct Collector::CollectorImpl
    {
        ItemMap m_map;
        ItemSet m_set;
    };
    
    Collector::Collector()
    : m_impl (new CollectorImpl)
    {
    }
    
    Collector::~Collector() = default;
    
    void Collector::Clear()
    {
        m_impl->m_map.clear();
        m_impl->m_set.clear();
    }
    
    Iterator* Collector::CreateIterator() const
    {
        return new IteratorImpl<ItemSet::const_iterator>(m_impl->m_set.cbegin(), m_impl->m_set.cend());
    }
    
    void Collector::Collect(Iterator* shape_iter, ExpandFunc expand_func)
    {
        for(;shape_iter->IsValid();shape_iter->Next())
        {
            // Expand current item
            auto cur_items = expand_func(shape_iter->Item());
            
            // Insert items
            m_impl->m_set.insert(cur_items.cbegin(), cur_items.cend());
        }
    }
    
    void Collector::Commit()
    {
        m_impl->m_map.clear();
        
        int idx = 0;
        for (auto& i : m_impl->m_set)
        {
            m_impl->m_map[i] = idx++;
        }
    }
    
    void Collector::Finalize(FinalizeFunc finalize_func)
    {
        for (auto& i : m_impl->m_set)
        {
            finalize_func(i);
        }
    }
    
    bool Collector::NeedsUpdate(Bundle const* bundle, ChangedFunc changed_func) const
    {
        auto bundle_impl = static_cast<BundleImpl const*>(bundle);
        
        // Check if:
        // 0) bundle and our map sizes match.
        // 1) All the objects collector has are in the bundle.
        // 2) They have not changed.
        if (bundle_impl->m_set.size() != m_impl->m_set.size())
        {
            return true;
        }
        
        for (auto& i : m_impl->m_set)
        {
            if (changed_func(i))
            {
                // Case 2: we have the object which is changed.
                return true;
            }
            
            auto iter = bundle_impl->m_set.find(i);
            
            if (iter == bundle_impl->m_set.cend())
            {
                // Case 1: we have an object which is not serialized as a part of bundle.
                return true;
            }

            // Here we know that bundle_impl->m_map[i] == m_impl->m_map[i]
            // since it is defined by set traversal order.
        }
        
        return false;
    }
    
    std::size_t Collector::GetNumItems() const
    {
        return m_impl->m_set.size();
    }
    
    Bundle* Collector::CreateBundle() const
    {
        return new BundleImpl { m_impl->m_map, m_impl->m_set };
    }
    
    std::uint32_t Collector::GetItemIndex(void const* item) const
    {
        auto iter = m_impl->m_map.find(item);
        
        if (iter == m_impl->m_map.cend())
        {
            throw std::runtime_error("No such item in the collector");
        }
        
        return iter->second;
    }
    
    
}
