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
#include "hip_intersection_device.h"

#include "buffer.h"
#include "device.h"
#include "event.h"
#include "../except/except.h"





namespace RadeonRays
{
    //simple RadeonRays::Buffer implementation
        HipIntersectionDevice::HipIntersectionDevice()
        {

        }

        HipIntersectionDevice::~HipIntersectionDevice()
        {

        }

        //IntersectionDevice
        void HipIntersectionDevice::Preprocess(World const& world)
        {

        }

        Buffer* HipIntersectionDevice::CreateBuffer(size_t size, void* initdata) const
        {
            return nullptr;
        }

        void HipIntersectionDevice::DeleteBuffer(Buffer* const) const
        {

        }

        void HipIntersectionDevice::DeleteEvent(Event* const) const
        {

        }

        void HipIntersectionDevice::MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const
        {

        }

        void HipIntersectionDevice::UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const
        {

        }

        void HipIntersectionDevice::QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const
        {

        }

        void HipIntersectionDevice::QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const
        {

        }

        void HipIntersectionDevice::QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitinfos, Event const* waitevent, Event** event) const
        {

        }

        void HipIntersectionDevice::QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitresults, Event const* waitevent, Event** event) const
        {

        }

} //RadeonRays