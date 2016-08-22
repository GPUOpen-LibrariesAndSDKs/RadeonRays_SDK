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

#ifndef INTERSECTION_DEVICE_H
#define INTERSECTION_DEVICE_H
#include "radeon_rays.h"

namespace RadeonRays
{
    class World;
    ///< The class represents a device capable of making intersection queries
    ///<
    class IntersectionDevice
    {
    public:
        IntersectionDevice() = default;

        virtual ~IntersectionDevice() = default;

        // Do all necessary scene preprocessing.
        // The call is blocking.
        virtual void Preprocess(World const& world) = 0;

        // Create a buffer of a specified size with specified initial data.
        // if initdata == nullptr the buffer is allocated, but not initialized.
        virtual Buffer* CreateBuffer(size_t size, void* initdata) const = 0;

        // Release buffer memory.
        virtual void DeleteBuffer(Buffer* const) const = 0;

        // Release an event (this method is optimized for frequent calls)
        virtual void DeleteEvent(Event* const) const = 0;

        // Map buffer data.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const = 0;

        // Unmap buffer data.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const = 0;

        // Find intersection for the rays in rays buffer and write them into hits buffer.
        // rays is assumed AOS with elements of type RadeonRays::ray.
        // hits is assumed AOS with elements of type RadeonRays::Intersection.
        // The call waits until waitevent is resolved (on a target device) if waitevent != nullptr.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void QueryIntersection(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const = 0;

        // Find if the rays in rays buffer intersect any of the primitives in the scene.
        // rays is assumed AOS with elements of type RadeonRays::ray.
        // hits is assumed AOS with elements of type int (-1 if no intersection, 1 otherwise).
        // The call waits until waitevent is resolved (on a target device) if waitevent != nullptr.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void QueryOcclusion(Buffer const* rays, int numrays, Buffer* hits, Event const* waitevent, Event** event) const = 0;

        // Find intersection for the rays in rays buffer and write them into hits buffer. Take the number of rays from the buffer in remote memory.
        // rays is assumed AOS with elements of type RadeonRays::ray.
        // numrays is assumed an array with a single int element.
        // hits is assumed AOS with elements of type RadeonRays::Intersection.
        // The call waits until waitevent is resolved (on a target device) if waitevent != nullptr.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const = 0;

        // Find if the rays in rays buffer intersect any of the primitives in the scene. Take the number of rays from the buffer in remote memory.
        // rays is assumed AOS with elements of type RadeonRays::ray.
        // numrays is assumed an array with a single int element.
        // hits is assumed AOS with elements of type RadeonRays::Intersection.
        // The call waits until waitevent is resolved (on a target device) if waitevent != nullptr.
        // The call is non-blocking if event is passed it, otherwise (event == nullptr) it is blocking.
        virtual void QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hits, Event const* waitevent, Event** event) const = 0;
    
        IntersectionDevice(IntersectionDevice const&) = delete;
        IntersectionDevice& operator = (IntersectionDevice const&) = delete;
    };
}



#endif // DUMMY_DEVICE_H
