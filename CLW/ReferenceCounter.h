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
#ifndef __CLW__ReferenceCounter__
#define __CLW__ReferenceCounter__

#include <iostream>

#ifdef __APPLE__
#define STDCALL
#include <OpenCL/OpenCL.h>
#elif WIN32
#define STDCALL __stdcall
#include <CL/cl.h>
#else
#define STDCALL
#include <CL/cl.h>
#endif

template <typename T, cl_int(STDCALL *Retain)(T), cl_int(STDCALL *Release)(T)>
class ReferenceCounter
{
public:
    typedef ReferenceCounter<T, Retain, Release> SelfType;
    ReferenceCounter() : object_(nullptr) {}
    explicit ReferenceCounter(T object) : object_(object)
    {
        RetainObject();
    }
    
    ~ReferenceCounter()
    {
        ReleaseObject();
    }
    
    ReferenceCounter(SelfType const& rhs)
    {
        
        object_ = rhs.object_;
        
        RetainObject();
    }
    
    SelfType& operator = (SelfType const& rhs)
    {
        if (&rhs != this)
        {
            ReleaseObject();
            
            object_ = rhs.object_;
            
            RetainObject();
        }
        
        return *this;
    }
    
    operator T() const
    {
        return object_;
    }
    
private:
    void RetainObject()  { if (object_) Retain(object_); }
    void ReleaseObject() { if (object_) Release(object_); }
    
    T object_;
};

template <>
class ReferenceCounter<cl_platform_id, nullptr, nullptr>
{
public:
    typedef ReferenceCounter<cl_platform_id, nullptr, nullptr> SelfType;
    ReferenceCounter() : object_(nullptr) {}
    explicit ReferenceCounter(cl_platform_id object) : object_(object)
    {
    }
    
    ~ReferenceCounter()
    {
    }
    
    ReferenceCounter(SelfType const& rhs)
    {
        object_ = rhs.object_;
    }
    
    SelfType& operator = (SelfType const& rhs)
    {
        if (&rhs != this)
        {
            object_ = rhs.object_;
        }
        
        return *this;
    }
    
    operator cl_platform_id() const
    {
        return object_;
    }
    
private:
    
    cl_platform_id object_;
};


#endif /* defined(__CLW__ReferenceCounter__) */
