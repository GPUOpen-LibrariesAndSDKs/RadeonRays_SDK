/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without __restrict__ion, including without limitation the rights
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
#include "executable_hipw.h"
#include <hip/hip_runtime.h>
#include "../except_clw.h"
#include "device_hip.h"

#include "intersect_hlbvh_stack.cu"

namespace Calc
{
    void HlbvhStackFunction::Execute(std::uint32_t queue, size_t global_size, size_t local_size) const
    {

        if (m_args.size() != 8)
        {
            throw ExceptionClw("Invalid args count:" + std::to_string(m_args.size()));
        }

        auto type = GetType();
        switch(type)
        {
        case kIntersect:
            hipLaunchKernel(HIP_KERNEL_NAME(intersect_main),
                               dim3(global_size/local_size, global_size/local_size, 1),
                               dim3(local_size,local_size,1),
                               0, 0,
                               m_args[0].buffer->GetDeviceBufferAs<bvh_node const>(),
                               m_args[1].buffer->GetDeviceBufferAs<bbox const>(),
                               m_args[2].buffer->GetDeviceBufferAs<float3 const>(),
                               m_args[3].buffer->GetDeviceBufferAs<Face const>(),
                               m_args[4].buffer->GetDeviceBufferAs<ray const>(),
                               m_args[5].buffer->GetDeviceBufferAs<int const>(),
                               m_args[6].buffer->GetDeviceBufferAs<int>(),
                               m_args[7].buffer->GetDeviceBufferAs<Intersection>());
            break;
        case kOcclude:
            hipLaunchKernel(HIP_KERNEL_NAME(occluded_main),
                               dim3(global_size/local_size, global_size/local_size, 1),
                               dim3(local_size,local_size,1),
                               0, 0,
                               m_args[0].buffer->GetDeviceBufferAs<bvh_node const>(),
                               m_args[1].buffer->GetDeviceBufferAs<bbox const>(),
                               m_args[2].buffer->GetDeviceBufferAs<float3 const>(),
                               m_args[3].buffer->GetDeviceBufferAs<Face const>(),
                               m_args[4].buffer->GetDeviceBufferAs<ray const>(),
                               m_args[5].buffer->GetDeviceBufferAs<int const>(),
                               m_args[6].buffer->GetDeviceBufferAs<int>(),
                               m_args[7].buffer->GetDeviceBufferAs<int>());
            break;
        }
    }
} //Calc