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
#ifndef FIRERAYS_CL_H
#define FIRERAYS_CL_H

#include "firerays.h"

#ifdef __APPLE__
#include <OpenCL/OpenCL.h>
#else
#include <CL/cl.h>
#endif

namespace FireRays
{
    class FRAPI IntersectionApiCL : public IntersectionApi
    {
    public:
        static IntersectionApiCL* CreateFromOpenClContext(cl_context context, cl_device_id device, cl_command_queue queue);

        /******************************************
        Memory management
        ******************************************/
        // Create a buffer from CL memory object
        virtual Buffer* CreateFromOpenClBuffer(cl_mem buffer) const = 0;

    protected:
        IntersectionApiCL();
        IntersectionApiCL(IntersectionApiCL const&);
        IntersectionApiCL const& operator = (IntersectionApiCL const&);
    };

    inline IntersectionApiCL::IntersectionApiCL(){}
}

#endif