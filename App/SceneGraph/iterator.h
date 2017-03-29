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
#pragma once 

/**
 \file iterator.h
 \author Dmitry Kozlov
 \version 1.0
 \brief Contains declaration of Baikal object iterators.
 */
namespace Baikal
{
    /** 
     \brief Iterator base interface.
     
     \details Iterators are used to go over different Baikal scene graph objects, such as meshes or lights.
     */
    class Iterator
    {
    public:
        // Constructor
        Iterator() = default;
        // Destructor
        virtual ~Iterator() = default;
        
        // Valid means it still has items to iterate over: you can call Next()
        virtual bool IsValid() const = 0;
        
        // Goes to next object, iterator should be valid before this call
        virtual void Next() = 0;
        
        // Retrieve underlying object
        virtual void const* Item() const = 0;

        // Sets the iterator into its initial state (beginning of the sequence)
        virtual void Reset() = 0;
        
        // Retrieve with uncoditional cast: caller is responsible of all the implications, no type check here
        template <typename T> T* ItemAs() const { return reinterpret_cast<T*>(Item()); }
        
        // Disable copies and moves
        Iterator(Iterator const&) = delete;
        Iterator& operator = (Iterator const&) = delete;
    };
    
    
    /**
     \brief Represents empty sequence.
     
     \details Objects return empty iterator when there is nothing to iterate over.
     */
    class EmptyIterator : public Iterator
    {
    public:
        // Constructor
        EmptyIterator() = default;
        // Destructor
        ~EmptyIterator() = default;
        // EmptyIterator is never valid
        bool IsValid() const override { return false; }
        // Nothing to go to
        void Next() override {}
        // Dereferencing always returns nullptr
        void const* Item() const override { return nullptr; }
        // Nothing to reset
        void Reset() override {}
    };
    
    /**
     \brief Represents wrapper on iterable entities supporting comparison and ++.
     
     \details Objects return this type of iterator if they are iteration over arrays or stl containers.
     */
    template <typename UnderlyingIterator> class IteratorImpl : public Iterator
    {
    public:
        // Initialize with an existing iterators
        IteratorImpl(UnderlyingIterator begin, UnderlyingIterator end)
        : m_begin(begin)
        , m_end(end)
        , m_cur(begin)
        {
        }
        
        // Check if we reached end
        bool IsValid() const override
        {
            return m_cur != m_end;
        }
        
        // Advance by 1
        void Next() override
        {
            ++m_cur;
        }
        
        // Get underlying item
        void const* Item() const override
        {
            return *m_cur;
        }
        
        // Set to starting iterator
        void Reset() override
        {
            m_cur = m_begin;
        }
        
    private:
        UnderlyingIterator m_begin;
        UnderlyingIterator m_end;
        UnderlyingIterator m_cur;
    };
    
    template <typename T> class ContainerIterator : public Iterator
    {
    public:
        ContainerIterator(T&& container) :
        m_container(std::move(container))
        {
            m_begin = m_container.cbegin();
            m_end = m_container.cend();
            Reset();
        }
        
        // Check if we reached end
        bool IsValid() const override
        {
            return m_cur != m_end;
        }
        
        // Advance by 1
        void Next() override
        {
            ++m_cur;
        }
        
        // Get underlying item
        void const* Item() const override
        {
            return *m_cur;
        }
        
        // Set to starting iterator
        void Reset() override
        {
            m_cur = m_begin;
        }
        
    private:
        T m_container;
        typename T::const_iterator m_begin;
        typename T::const_iterator m_end;
        typename T::const_iterator m_cur;
    };
}
