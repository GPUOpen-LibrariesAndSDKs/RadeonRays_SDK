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
#ifndef INTERSECTIONAPI_IMPL
#define INTERSECTIONAPI_IMPL

#include <atomic>

#include "radeon_rays.h"
#include "../world/world.h"

namespace RadeonRays
{
    class IntersectionDevice;

    /// IntersectionApi is designed to provide fast means for ray-scene intersection
    /// for AMD architectures. It effectively absracts underlying AMD hardware and
    /// software stack and allows user to issue low-latency batched ray queries.
    /// The API has 2 major workflows:
    ///    - Fast path: the data is expected to be in host memory as well as the returned result is put there
    ///    - Complete path: the data can be put into remote memory allocated with API and can be accessed.
    ///      by the app directly in remote memory space.
    ///
    class IntersectionApiImpl : public IntersectionApi
    {
    public:
        /******************************************
        Geometry manipulation
        ******************************************/
        // Fast path functions to create entities from host memory

        // The mesh might be mixed quad\triangle mesh which is determined
        // by numfacevertices array containing numfaces entries describing
        // the number of vertices for current face (3 or 4)
        // The call is blocking, so the returned value is ready upon return.
        Shape* CreateMesh(
            // Position data
            float const * vertices, int vnum, int vstride,
            // Index data for vertices
            int const * indices, int istride,
            // Numbers of vertices per face
            int const * numfacevertices,
            // Number of faces
            int  numfaces
            ) const override;

        // Create an instance of a shape with its own transform (set via Shape interface).
        // The call is blocking, so the returned value is ready upon return.
        Shape* CreateInstance(Shape const* shape) const override;
        // Delete the shape (to simplify DLL boundary crossing
        void DeleteShape(Shape const* shape) override;
        // Attach shape to participate in intersection process
        void AttachShape(Shape const* shape) override;
        // Detach shape, i.e. it is not going to be considered part of the scene anymore
        void DetachShape(Shape const* shape) override;
        // Detach all objects
        void DetachAll() override;
        // Commit all geometry creations/changes
        void Commit() override;

        //Sets the shape id allocator to its default value (1)
        void ResetIdCounter() override;
        //Returns true if no shapes are in the world
        bool IsWorldEmpty() override;

        /******************************************
        Memory management
        ******************************************/
        Buffer* CreateBuffer(size_t size, void* initdata) const override;

        // Delete the buffer
        void DeleteBuffer(Buffer* buffer) const override;
        // Map buffer. Event pointer might be nullptr.
        // The call is asynchronous.
        void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const override;
        // Unmap buffer
        void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const override;

        /******************************************
          Events handling
        *******************************************/
        void DeleteEvent(Event* event) const override;

        /******************************************
        Ray casting
        ******************************************/
        // Complete path:
        // Find closest intersection
        // TODO: do we need to modify rays' intersection range?
        // TODO: SoA vs AoS?
        // The call is asynchronous. Event pointers might be nullptrs.
        void QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;
        // Find any intersection.
        // The call is asynchronous. Event pointer mights be nullptrs.
        void QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;

        // Find closest intersection, number of rays is in remote memory
        // TODO: do we need to modify rays' intersection range?
        // TODO: SoA vs AoS?
        // The call is asynchronous. Event pointers might be nullptrs.
        void QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitinfos, Event const* waitevent, Event** event) const override;
        // Find any intersection.
        // The call is asynchronous. Event pointer mights be nullptrs.
        void QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitresults, Event const* waitevent, Event** event) const override;

        /******************************************
        Utility
        ******************************************/
        // Set API global option: string
        void SetOption(char const* name, char const* value) override;
        // Set API global option: float
        void SetOption(char const* name, float value) override;
        

        IntersectionDevice* GetDevice() const { return m_device.get(); }

        IntersectionApiImpl(IntersectionDevice* device);
    protected:
        friend class IntersectionApi;

        ~IntersectionApiImpl();

    private:
        // Container for all shapes
        World world_;
        // Shape ID tracker
        mutable std::atomic<Id> nextid_;
        // Intersection device
        std::unique_ptr<IntersectionDevice> m_device;
    };
}




#endif
