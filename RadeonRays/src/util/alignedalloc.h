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
#ifndef ALIGNEDALLOC_H
#define ALIGNEDALLOC_H

#include <memory>

namespace RadeonRays {

    /**
    * STL-compliant allocator that allocates aligned memory.
    * @tparam T Type of the element to allocate.
    * @tparam Alignment Alignment of the allocation, e.g. 16.
    * @ingroup AlignedAllocator
    */
    template <class T, size_t Alignment>
    struct aligned_allocator
        : public std::allocator<T> // Inherit construct(), destruct() etc.
    {
#if 0
        typedef size_t    size_type;
        typedef ptrdiff_t difference_type;
        typedef T*        pointer;
        typedef const T*  const_pointer;
        typedef T&        reference;
        typedef const T&  const_reference;
        typedef T         value_type;
#endif
        using size_type = typename std::allocator<T>::size_type;
        using pointer = typename std::allocator<T>::pointer;
        using const_pointer = typename std::allocator<T>::const_pointer;

        /// Defines an aligned allocator suitable for allocating elements of type
        /// @c U.
        template <class U>
        struct rebind { using other = aligned_allocator<U, Alignment>; };

        /// Default-constructs an allocator.
        aligned_allocator() throw() = default;

        /// Copy-constructs an allocator.
        aligned_allocator(const aligned_allocator& other) throw()
            : std::allocator<T>(other) { }

        /// Convert-constructs an allocator.
        template <class U>
        aligned_allocator(const aligned_allocator<U, Alignment>&) throw() { }

        /// Destroys an allocator.
        ~aligned_allocator() throw() = default;

        /// Allocates @c n elements of type @c T, aligned to a multiple of
        /// @c Alignment.
        pointer allocate(size_type n)
        {
            return allocate(n, const_pointer(0));
        }

        /// Allocates @c n elements of type @c T, aligned to a multiple of
        /// @c Alignment.
        pointer allocate(size_type n, const_pointer /* hint */)
        {
            void *p;
#ifndef _WIN32
            if (posix_memalign(&p, Alignment, n*sizeof(T)) != 0)
                p = NULL;
#else
            p = _aligned_malloc(n*sizeof(T), Alignment);
#endif
            if (!p)
                throw std::bad_alloc();
            return static_cast<pointer>(p);
        }

        /// Frees the memory previously allocated by an aligned allocator.
        void deallocate(pointer p, size_type /* n */)
        {
#ifndef _WIN32
            free(p);
#else
            _aligned_free(p);
#endif
        }
    };

    /**
    * Checks whether two aligned allocators are equal. Two allocators are equal
    * if the memory allocated using one allocator can be deallocated by the other.
    * @returns Always @c true.
    * @ingroup AlignedAllocator
    */
    template <class T1, size_t A1, class T2, size_t A2>
    bool operator == (const aligned_allocator<T1, A1> &, const aligned_allocator<T2, A2> &)
    {
        return true;
    }

    /**
    * Checks whether two aligned allocators are not equal. Two allocators are equal
    * if the memory allocated using one allocator can be deallocated by the other.
    * @returns Always @c false.
    * @ingroup AlignedAllocator
    */
    template <class T1, size_t A1, class T2, size_t A2>
    bool operator != (const aligned_allocator<T1, A1> &, const aligned_allocator<T2, A2> &)
    {
        return false;
    }

} // namespace util

#endif // UTILITIES_ALIGNED_ALLOCATOR_HPP