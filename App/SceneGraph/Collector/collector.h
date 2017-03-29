/**********************************************************************
 Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 ********************************************************************/

/**
 \file collector.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains collector related classes.
 */
#pragma once
#include <memory>
#include <map>
#include <set>
#include <functional>


namespace Baikal
{
    class Material;
    class Iterator;
    
    /**
     \brief Serialized bundle.
     
     Bundle is a representation of a chunk of serialized memory keeping pointer to offset map.
     */
    class Bundle
    {
    public:
        virtual ~Bundle() = 0;
    };

    /**
     \brief Collector class.
     
     Collector iterates over collection of objects collecting objects and their dependecies into random access bundle.
     The engine uses collectors in order to resolve material-texture or shape-material dependecies for GPU serialization.
     */
    class Collector
    {
    public:
        using ExpandFunc = std::function<std::set<void const*>(void const*)>;
        using ChangedFunc = std::function<bool(void const*)>;
        using FinalizeFunc = std::function<void(void const*)>;
        
        // Constructor
        Collector();
        // Destructor
        virtual ~Collector();
        
        // Clear collector state (CreateIterator returns invalid iterator if the collector is empty)
        void Clear();
        // Create an iterator of objects
        Iterator* CreateIterator() const;
        // Collect objects and their dependencies
        void Collect(Iterator* iter, ExpandFunc expand_func);
        // Commit collected objects
        void Commit();
        // Given a budnle check if all collected objects are in the bundle and do not require update
        bool NeedsUpdate(Bundle const* bundle, ChangedFunc cahnged_func) const;
        // Get number of objects in the collection
        std::size_t GetNumItems() const;
        // Create serialised bundle (randomly accessible dump of objects)
        Bundle* CreateBundle() const;
        // Get item index within a collection
        std::uint32_t GetItemIndex(void const* item) const;
        // Finalization function
        void Finalize(FinalizeFunc finalize_func);
    
        // Disallow copies and moves
        Collector(Collector const&) = delete;
        Collector& operator = (Collector const&) = delete;
        
    private:
        struct CollectorImpl;
        std::unique_ptr<CollectorImpl> m_impl;
    };
    
    inline Bundle::~Bundle()
    {
    }
}


