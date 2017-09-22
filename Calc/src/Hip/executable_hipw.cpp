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
#include <vector>
#include <string>
#include "hip/hip_runtime.h"
#include "device_hip.h"
#include "buffer.h"
#include "event.h"
#include "../except_clw.h"
#include "executable_hipw.h"


namespace Calc
{
    HipFunction::HipFunction(Type type) : m_type(type)
    {

    }
    HipFunction::~HipFunction()
    {
        m_args.clear();
    };

    void HipFunction::SetArg(std::uint32_t idx, std::size_t arg_size, void* arg)
    {
        Resize(idx + 1);
        m_args[idx].type = Arg::kPointer;
        m_args[idx].pointer = arg;

    }
    
    void HipFunction::SetArg(std::uint32_t idx, Buffer const* arg)
    {
        HipBuffer const* buff = dynamic_cast<HipBuffer const*>(arg);
        assert(buff);
        Resize(idx + 1);

        m_args[idx].type = Arg::kBuffer;
        m_args[idx].buffer = buff;
    }
    
    void HipFunction::SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem)
    {
        Resize(idx + 1);
        
        m_args[idx].type = Arg::kSharedMem;
        m_args[idx].mem = shmem;
    }

    std::vector<HipFunction::Arg>& HipFunction::GetArgs()
    {
        return m_args;
    }

    HipFunction::Type HipFunction::GetType() const
    { 
        return m_type;
    };
    
    void HipFunction::Resize(std::size_t size)
    {
        if (m_args.size() < size)
        {
            m_args.resize(size);
        }
    }

    HipExecutable::HipExecutable(const std::string& prefix)
     : m_prefix(prefix)
    {

    }

    HipExecutable::~HipExecutable()
    {

    }

    Function* HipExecutable::CreateFunction(char const* name)
    {
        std::string func_name = name;
        HipFunction::Type type = HipFunction::kIntersect;
        Function* result = nullptr;
        if (name == "occluded_main")
        {
            type = HipFunction::kOcclude;
        }

        if (m_prefix == "bvh2_bittrail")
        {
            result = new Bhv2BittrailFunction(type);
        }
        else if (m_prefix == "bvh2level_skiplinks")
        {
            result = new Bvh2levelSkiplinksFunction(type);
        }
        else if (m_prefix == "bvh2_short_stack")
        {
            result = new Bvh2ShortstackFunction(type);
        }
        else if (m_prefix == "bvh2_skiplinks")
        {
            result = new Bvh2SkiplinksFunction(type);
        }
        else if (m_prefix == "hlbvh_stack")
        {
            result = new HlbvhStackFunction(type);
        }
        else
        {
            throw ExceptionClw("Invalid function prefix:" + m_prefix);
        }

        return result;
    }
    
    void HipExecutable::DeleteFunction(Function* func)
    {
        delete func;
    }
} //Calc
