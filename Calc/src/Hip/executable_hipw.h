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

#include <vector>
#include "executable.h"
#include "calc_common.h"
#include "device.h"

namespace Calc
{
    class HipBuffer;
    class HipFunction: public Function
    {
    public:
        struct Arg
        {
            enum
            {
                kPointer = 0,
                kBuffer,
                kSharedMem
            } type;

            union
            {
                void const* pointer;
                HipBuffer const* buffer;
                SharedMemory mem;
            };
        };
        enum Type
        {
            kIntersect = 0,
            kOcclude
        };
        HipFunction(Type type);
        virtual ~HipFunction();

        // Argument setters
        virtual void SetArg(std::uint32_t idx, std::size_t arg_size, void* arg) override;
        virtual void SetArg(std::uint32_t idx, Buffer const* arg) override;
        virtual void SetArg(std::uint32_t idx, std::size_t size, SharedMemory shmem) override;

        std::vector<Arg>& GetArgs();

        Type GetType() const;

        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const = 0;
    
    protected:
        void Resize(std::size_t size);
        Type m_type;
        std::vector<Arg> m_args;
    };

    // Executable which is capable of being launched on a device
    class HipExecutable : public Executable
    {
    public:
        HipExecutable(const std::string& prefix);
        virtual ~HipExecutable();

        // Function management
        virtual Function* CreateFunction(char const* name) override;
        virtual void DeleteFunction(Function* func) override;

    private:
        std::string m_prefix;
    };

    //wrap kernels:
    //bvh2_bittrail
    class Bhv2BittrailFunction: public HipFunction
    {
    public:
        Bhv2BittrailFunction(Type t) : HipFunction(t){};
        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const override;
    };

    //bvh2level_skiplinks
    class Bvh2levelSkiplinksFunction: public HipFunction
    {
    public:
        Bvh2levelSkiplinksFunction(Type t) : HipFunction(t){};

        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const override;
    };

    //bvh2_short_stack
    class Bvh2ShortstackFunction: public HipFunction
    {
    public:
        Bvh2ShortstackFunction(Type t) : HipFunction(t){};
        
        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const override;
    };

    //bvh2_skiplinks
    class Bvh2SkiplinksFunction: public HipFunction
    {
    public:
        Bvh2SkiplinksFunction(Type t) : HipFunction(t){};
        
        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const override;
    };

    //hlbvh_stack
    class HlbvhStackFunction: public HipFunction
    {
    public:
        HlbvhStackFunction(Type t) : HipFunction(t){};
        
        virtual void Execute(std::uint32_t queue, size_t global_size, size_t local_size) const override;
    };
}