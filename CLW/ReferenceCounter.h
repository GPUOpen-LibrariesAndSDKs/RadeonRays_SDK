/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
