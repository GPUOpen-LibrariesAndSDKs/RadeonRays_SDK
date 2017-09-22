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

#include "intersection_device.h"


namespace RadeonRays
{
    class Mesh;
    class Shape;
    class ShapeImpl;
    ///< The class represents Embree based intersection device.
    ///< It uses embree RTCDevice abstraction to implement intersection algorithm.
    ///<
    class HipIntersectionDevice : public IntersectionDevice
    {
    public:
        //
        HipIntersectionDevice();
        ~HipIntersectionDevice();

        //IntersectionDevice
        void Preprocess(World const& world) override;
        Buffer* CreateBuffer(size_t size, void* initdata) const override;
        void DeleteBuffer(Buffer* const) const override;
        void DeleteEvent(Event* const) const override;
        void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const override;
        void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const override;
        void QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;
        void QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;
        void QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;
        void QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;
    
    protected:

    };
}

