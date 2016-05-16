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

#ifndef FIRERAYS_H
#define FIRERAYS_H

#include "math/float3.h"
#include "math/float2.h"
#include "math/matrix.h"
#include "math/ray.h"
#include "math/mathutils.h"

#define FIRERAYS_API_VERSION 2.0


#ifdef WIN32
    #ifdef EXPORT_API
        #define FRAPI __declspec(dllexport)
    #else
        #define FRAPI __declspec(dllimport)
    #endif
#elif defined(__GNUC__)
    #ifdef EXPORT_API
        #define FRAPI __attribute__((visibility ("default")))
    #else
        #define FRAPI
    #endif
#endif

namespace FireRays 
{
    /// Represents a device, which can be used by API for intersection purposes.
    /// API is distributing the work across multiple devices itsels, so
    /// this structure is only used to query devices configuration and
    /// limit the number of devices available for the API.
    struct FRAPI DeviceInfo
    {
        // Device type
        enum Type
        {
            kCpu,
            kGpu,
            kAccelerator
        };

        // Device name
        char const* name;
        // Device vendor
        char const* vendor;
        // Device type
        Type type;
    };

    // Forward declaration of entities
    typedef int Id;
    const Id kNullId = -1;

    // Shape interface to repesent intersectable entities
    // The shape is assigned a particular ID which
    // is put by an intersection engine into Intersection structure
    class FRAPI Shape
    {
    public:
        virtual ~Shape() = 0;

        // World space transform
        virtual void SetTransform(matrix const& m, matrix const& minv) = 0;
        virtual void GetTransform(matrix& m, matrix& minv) const = 0;

        // Motion blur
        virtual void SetLinearVelocity(float3 const& v) = 0;
        virtual float3 GetLinearVelocity() const = 0;

        virtual void SetAngularVelocity(quaternion const& q) = 0;
        virtual quaternion GetAngularVelocity() const = 0;

        // ID of a shape
        virtual void SetId(Id id) = 0;
        virtual Id GetId() const = 0;

		// Geometry mask to mask out intersections
		virtual void SetMask(int mask) = 0;
		virtual int  GetMask() const = 0;
    };

    // Buffer represents a chunk of memory hosted inside the API
    // (or extrenally in case of interop)
    class FRAPI Buffer
    {
    public:
        virtual ~Buffer() = 0;
    };

    // Synchronization object returned by some API routines.
    class FRAPI Event
    {
    public:
        virtual ~Event() = 0;
        // Indicates whether the related action has been completed
        virtual bool Complete() const = 0;
        // Blocks execution until the event is completed
        virtual void Wait() = 0;
    };

    // Exception class
    class FRAPI Exception
    {
    public:
        virtual ~Exception() = 0;
        virtual char const* what() const = 0;
    };

    struct Intersection
    {
        // UV parametrization
        float4 uvwt;
        // Shape ID
        Id shapeid;
        // Primitve ID
        Id primid;

        int padding0;
        int padding1;

        Intersection();
    };

    enum MapType
    {
        kMapRead = 0x1,
        kMapWrite = 0x2
    };

    // IntersectionApi is designed to provide fast means for ray-scene intersection
    // for AMD architectures. It effectively absracts underlying AMD hardware and
    // software stack and allows user to issue low-latency batched ray queries.
    // The API has 2 major workflows:
    //    - Fast path: the data is expected to be in host memory as well as the returned result is put there
    //    - Complete path: the data can be put into remote memory allocated with API and can be accessed.
    //      by the app directly in remote memory space.
    //
    class FRAPI IntersectionApi
    {
    public:

        /******************************************
        Device management
        ******************************************/
        // Use this part of API to query for available devices and
        // limit the set of devices which API is going to use
        // Get the number of devices available in the system
        static std::uint32_t GetDeviceCount();
        // Get the information for the specified device
        static void GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo);

        /******************************************
        API lifetime management
        ******************************************/
        static IntersectionApi* Create(std::uint32_t devidx);

        // Deallocation
        static void Delete(IntersectionApi* api);

        /******************************************
        Geometry manipulation
        ******************************************/
        // Fast path functions to create entities from host memory

        // The mesh might be mixed quad\triangle mesh which is determined
        // by numfacevertices array containing numfaces entries describing
        // the number of vertices for current face (3 or 4)
        // The call is blocking, so the returned value is ready upon return.
        virtual Shape* CreateMesh(
            // Position data
            float* vertices, int vnum, int vstride,
            // Index data for vertices
            int* indices, int istride,
            // Numbers of vertices per face
            int* numfacevertices,
            // Number of faces
            int  numfaces
            ) const = 0;

        // Create an instance of a shape with its own transform (set via Shape interface).
        // The call is blocking, so the returned value is ready upon return.
        virtual Shape* CreateInstance(Shape const* shape) const = 0;
        // Delete the shape (to simplify DLL boundary crossing
        virtual void DeleteShape(Shape const* shape) = 0;
        // Attach shape to participate in intersection process
        virtual void AttachShape(Shape const* shape) = 0;
        // Detach shape, i.e. it is not going to be considered part of the scene anymore
        virtual void DetachShape(Shape const* shape) = 0;
        // Commit all geometry creations/changes
        virtual void Commit() = 0;

        /******************************************
        Memory management
        ******************************************/
        // Create a buffer to use the most efficient acceleration possible
        virtual Buffer* CreateBuffer(size_t size, void* initdata) const = 0;
        // Delete the buffer
        virtual void DeleteBuffer(Buffer* buffer) const = 0;
        // Map buffer. Event pointer might be nullptr.
        // The call is asynchronous.
        virtual void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const = 0;
        // Unmap buffer
        virtual void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const = 0;

        /******************************************
          Events handling
        *******************************************/
        virtual void DeleteEvent(Event* event) const = 0;

        /******************************************
          Ray casting
        ******************************************/
        // Complete path:
        // Find closest intersection
        // The call is asynchronous. Event pointers might be nullptrs.
        virtual void QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const = 0;
        // Find any intersection.
        // The call is asynchronous. Event pointer mights be nullptrs.
        virtual void QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const = 0;

        // Find closest intersection, number of rays is in remote memory
        // The call is asynchronous. Event pointers might be nullptrs.
        virtual void QueryIntersection(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitinfos, Event const* waitevent, Event** event) const = 0;
        // Find any intersection.
        // The call is asynchronous. Event pointer mights be nullptrs.
        virtual void QueryOcclusion(Buffer const* rays, Buffer const* numrays, int maxrays, Buffer* hitresults, Event const* waitevent, Event** event) const = 0;

        /******************************************
        Utility
        ******************************************/
        // Set API global option: string
        // Supported options:
        // option "bvh.type" values {"bvh" (regular bvh, default), "qbvh" (4 branching factor), "hlbvh" (fast builds)}
        // option "bvh.force2level" values {0(default), 1}
        //         by default 2-level BVH is used only if there is instancing in the scene or
        //         motion blur is enabled. 1 forces 2-level BVH for all cases.
        // option "bvh.builder" values {"sah" (use surface area heuristic), "median" (use spatial median, faster to build, default)}
        // option "bvh.sah.usesplits" values {0(default),1} (allow spatial splits for BVH)
        // option "bvh.sah.trisah" values {float, default = 0.01f for GPU } (cost of triangle intersection vs node traversal)
        // option "bvh.sah.overlaparea" values { float < 1.f, default = 0.0001f } 
        //         (overlap area which is considered for a spatial splits, fraction of parent bbox)
        // option "bvh.sah.maxprimsperleaf" values {int, default = 4} (the limit on number of primitives per BVH leaf)
        // option "bvh.sah.maxdepth" values {int, default = 10} (max depth in the tree where spatial split can happen)
        virtual void SetOption(char const* name, char const* value) = 0;
        // Set API global option: float
        virtual void SetOption(char const* name, float value) = 0;

    protected:
        IntersectionApi();
        IntersectionApi(IntersectionApi const&);
        IntersectionApi& operator = (IntersectionApi const&);

        virtual ~IntersectionApi() = 0;
    };

    inline IntersectionApi::IntersectionApi(){}
    inline IntersectionApi::~IntersectionApi(){}

    inline Intersection::Intersection()
        : shapeid(kNullId)
        , primid(kNullId)
    {
    }

    inline Buffer::~Buffer(){}
    inline Shape::~Shape(){}
    inline Event::~Event(){}
    inline Exception::~Exception(){}
}

#endif // INTERSECTIONAPI_H
