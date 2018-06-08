## Getting started with the API

The concept of API and its workflow is relatively simple. To use the API,
include the following header:
```
#include "radeon_rays.h"
```

All classes and functions in RadeonRays are defined in the corresponding
namespace, so to access them the user explicitly specifies the namespace or
inserts the following using statement:
```
using namespace RadeonRays;
```

#### Device enumeration
Next the user configures the intersection devices by means of the RadeonRays
device enumeration API. RadeonRays exposes all devices capable of performing
intersection queries with the following static methods of IntersectionApi
class:
```
// Get the number of devices available in the system
static std::uint32_t GetDeviceCount();
// Get the information for the specified device
static void GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo);
```

Devices are identified by their indices, so the user needs to check device
capabilities and save each device’s index somewhere if the device fits user
needs. For example, you can use the following construct to choose the GPU OpenCL
device using enumeration API:
```
int deviceidx = -1;
for (auto idx = 0U; idx < IntersectionApi::GetDeviceCount(); ++idx)
{
    DeviceInfo devinfo;
    IntersectionApi::GetDeviceInfo(idx, devinfo);

    if (devinfo.type == DeviceInfo::kGpu && devinfo.platform == DeviceInfo::kOpenCL)
    {
        deviceidx = idx;
    }
}
```

Optionally, the user can specify platform of listed devices:
```
IntersectionApi::SetPlatform(DeviceInfo::kOpenCL);
```
#### API Initialization
Next the user creates an API instance based on the device chosen in previous
step, as shown in the following code:
```
    IntersectionApi* api = IntersectionApi::Create(deviceidx);
```

There is a chance an error will occur during library initialization (OpenCL
runtime issue, access rights problem, etc.). To communicate the reason back to
the user, the library uses a derivative of the Exception interface so the user
can catch it and get the text description of the error.

#### Geometry creation
Now that the user has an instance of the API, he or she can move to the geometry
creation stage. The current version of the API supports triangle, quad, and
mixed meshes along with the instancing. The following snippet shows how to
create a simple mesh consisting of a single triangle:
```
// Mesh vertices
float const vertices[] = {
    -1.f,-1.f,0.f,
    1.f,-1.f,0.f,
    0.f,1.f,0.f,
};
int const g_indices[] = { 0, 1, 2 };
// Number of vertices for the face
const int g_numfaceverts[] = { 3 };

Shape* shape = api->CreateMesh(g_vertices, 3, 3 * sizeof(float), g_indices, 0, g_numfaceverts, 1);
```
Here 0 is used for index stride, meaning indices are densely packed. The method
is blocking. However, it is safe to call from multiple threads as long as these
calls are not interleaved with ray casting method calls.  
The next step is to construct the scene from multiple meshes. To add/remove
meshes from the current scene (which is implicitly defined by API instance), you
can use the following methods:
 ```
 api->AttachShape(shape);
 api->DetachShape(shape);
 ```
These methods are fast since they are not going to launch any time-consuming
operations. Instead actual data transfers and acceleration structure
constructions are deferred till the call to Commit() method. This method should
be called any time something any time something has changed in the scene:
```
api->Commit();
```
The method is blocking and can’t be called simultaneously with other API methods.
#### Instancing
The geometry can be instanced in the API, which means the same base geometry is
used for different entities with different world transforms. The following code
should be used to instantiate the mesh:
```
Shape* instance = api->CreateInstance(shape);
```
Instance can be attached to the scene the same as any other regular shape.
Instancing allows you to create overwhelmingly complex scenes with a moderate
memory footprint by means of geometry reuse.
#### Memory API
Buffers are the way data is transferred to RadeonRays and back.  
Before the user will make query intersection, need to prepare rays data:
```
ray rays[3];
rays[0].o = float4(0.f, 0.f, -1.f, 1000.f);
rays[0].d = float3(0.f, 0.f, 10.f);
rays[1].o = float4(0.f, 0.5f, -10.f, 1000.f);
rays[1].d = float3(0.f, 0.f, 1.f);
rays[2].o = float4(0.4f, 0.f, -10.f, 1000.f);
rays[2].d = float3(0.f, 0.f, 1.f);
Buffer* ray_buffer = api->CreateBuffer(3 * sizeof(ray), rays);
```
The layout of ray is structure:  
* o.xyz - Ray origin
* d.xyz - Ray direction
* o.w   - Ray maximum distance
* d.w   - Time stamp for motion blur

The same way need to allocate memory for intersection results:
```
Buffer* isect_buffer = api->CreateBuffer(3 * sizeof(Intersection), nullptr);
```
Intersection structure:  
* uvwt.xyz - Parametric coordinates of a hit (xy for triangles and quads)
* uvwt.w   - Hit distance along the ray
* shapeid  - ID of a shape
* primid   - ID of a primitive within a shape  

Shape ID corresponds to a value which is either automatically assigned to a shape at creation time by the API or manually set by the user using Shape::SetId() method. Primitive ID is a zero-based index of a primitive within a shape (in the order they were passed to CreateMesh method). If no intersection is detected, they are both set to kNullId(-1).

Buffers can be mapped and unmapped with the following calls:
```
virtual void MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const = 0;
virtual void UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const = 0;
```
Note that these operations are asynchronous and you need to establish correct
dependencies to intersection queries to ensure they work as intended.
#### Simple intersection queries
As soon as the geometry is committed, the user can perform intersection queries.
As the library is specifically designed for heterogeneous architectures, it
accepts batches of rays rather than individual rays as an input to intersection
query methods. As a general rule, the larger the batch the better because
massively parallel devices can maintain better occupancy and perform better
latency hiding.  
There are basically two types of intersection queries:
 * Closest hit query (intersection)
 * Any hit query (occlusion)
The methods for querying closest hit are called QueryIntersection() and methods
for occlusion are called QueryOcclusion() (occlusion predicate).  
The simplest version of an intersection query looks like this:
```
api->QueryIntersection(ray_buffer, 3, isect_buffer, nullptr, nullptr);
```
An occlusion query has the same format but returns an array of integers, where
each entry is either -1 (no intersection) or 1 (intersection).
#### Asynchronous queries
The methods discussed above are blocking, but there is an option to launch a ray query asynchronously using the following version of the method:
```
// Intersect
Event* event = nullptr;
api->QueryIntersection(ray_buffer, numrays, isect_buffer, nullptr, &event);
```
This method launches an asynchronous query returning the pointer to an Event
object. This event can be used to track the status of execution or to build
dependency chains. To track the status, the user can use the following methods:
```
event->Complete();
event->Wait();
```
The first call returns true if the method has completed and the contents of the
result buffer are available. The second waits until execution is complete.  
In addition, you can pass the Event object to another ray query, thus
establishing a dependency. The second ray query in this case would start only
after the first one has finished:
```
api->QueryIntersection(ray_buffer, numrays, isect_buffer, event, nullptr);
```
#### OpenCL interop
There is a way to use existing OpenCL contexts in the API as well as to share
existing OpenCL buffers with the application code.  
Include RadeonRays OpenCL API header:
```
#include "radeon_rays_cl.h"
```
To create an API instance using an existing OpenCL context, the user can call
the following:
```
Cl_int status = clGetDeviceIDs(platform[0], type, 1, &device, nullptr);
cl_context rawcontext = clCreateContext(nullptr, 1, &device, nullptr, nullptr, &status);
cl_command_queue queue = clCreateCommandQueue(rawcontext, device, 0, &status);
api = RadeonRays::CreateFromOpenClContext(rawcontext, &device, &queue);
```
To share the buffer, you can use this code:
```
cl_mem rays_buffer = clCreateBuffer(rawcontext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, sizeof(ray), &r, &status);
Buffer* rays = CreateFromOpenClBuffer(api, rays_buffer);
```
#### Global options
Options control various aspects of RadeonRays behavior. Currently, they are mainly
used to control acceleration structure construction and traversal algorithms.
Refer to the header file for the complete set of supported options.  
For example, to set an option you use this call:
```
api->SetOption("bvh.builder", "sah");
```
#### Acceleration strategies
The default acceleration structure is Bounding Volume Hierarchy (BVH) using
spatial median splits. It maintains fast build times and provides decent
intersection performance. You can enable SAH builder using the global option and
trade off construction time for better intersection performance.  
If you need faster refits (for example if geometry is frequently changing
position), you can enable two level BVH, which doesn’t get re-created every
time the geometry transform is changed.  
For scenes containing instances or motion blur, two level BVH is used by default.

#### Releasing entities
All the entities created via the FireRays interface should be released when an
application shuts down. The following methods are available to release shapes,
buffers, and events:
```
api->DeleteShape(shape);
api->DeleteBuffer(buffer);
api->DeleteEvent(event);
```
Destroy the API instance itself with:
```
IntersectionApi::Delete(api);
```

## API reference
#### Backend management
```
static void IntersectionApi::SetPlatform(const DeviceInfo::Platform platform);
```
By default RadeonRays will any platform with potential GPU accelerations, if you
prefer to specify which platform call SetPlatform before GetDeviceInfo/GetDeviceCount
for each specific platform. By default will choose OpenCL if available, and if
not Vulkan.  
* *platform* - desired platform.

```
static std::uint32_t IntersectionApi::GetDeviceCount();
```
Return the number of devices available in the system for chosen platform(see
  SetPlatform).

```
static void IntersectionApi::GetDeviceInfo(std::uint32_t devidx, DeviceInfo& devinfo);
```
Get the information for the specified device.  
* *devidx* - device index(see GetDeviceCount)
* *devinfo* - output for the device information.

#### API lifetime management
```
static IntersectionApi* IntersectionApi::Create(std::uint32_t devidx);
```
Create api for specified device. Return the api.  
* *devidx* - device index(see GetDeviceCount)
```
static void IntersectionApi::Delete(IntersectionApi* api);
```
Deallocation of api.  
* *api* - api to delete(see Create method)

#### Geometry manipulation
```
virtual Shape* IntersectionApi::CreateMesh(
           float const * vertices, int vnum, int vstride,
           int const * indices, int istride,
           int const * numfacevertices,
           int  numfaces
           ) const = 0;
 ```
The mesh might be mixed quad/triangle mesh which is determined by
numfacevertices array containing numfaces entries describing the number of
vertices for current face (3 or 4).  
The call is blocking, so the returned value is ready upon return.  
* *vertices* - vertices array on host memory.
* *vnum* - vertices count.
* *vstride* - stride in *vertices* array.
* *indices* - indices array on host memory.
* *istride* - stride in *indices* array.
* *numfacevertices* - array containingumber of vertices for current face (3
  or 4). Can be nullptr if mesh contain only triangles.
* *numfaces* - count of primitives in mesh.

```
Shape* IntersectionApi::CreateInstance(Shape const* shape) const override;
```
Create an instance of a shape with its own transform (set via Shape interface).  
The call is blocking, so the returned value is ready upon return.  
* *shape* - the shape to create instance of.
```
void IntersectionApi::DeleteShape(Shape const* shape) override;
```
Delete the shape  
* *shape* - the shape to delete.
```
void IntersectionApi::AttachShape(Shape const* shape) override;
```
Attach shape to participate in intersection process(see IntersectionApi::Commit()).  
* *shape* - the shape to add to scene.
```
void IntersectionApi::DetachShape(Shape const* shape) override;
```
Detach shape, i.e. it is not going to be considered part of the scene
anymore(see IntersectionApi::Commit()).  
* *shape* - the shape to remove from scene.
```
void IntersectionApi::DetachAll() override;
```
Detach all objects(see IntersectionApi::Commit()).
```
void IntersectionApi::Commit() override;
```
Commit all geometry creations/changes

```
void IntersectionApi::ResetIdCounter() override;
```
Sets the shape id allocator to its default value (1)
```
bool IntersectionApi::IsWorldEmpty() override;
```
Returns true if no shapes are in the world

```
virtual void Shape::SetTransform(matrix const& m, matrix const& minv) = 0;
virtual void Shape::GetTransform(matrix& m, matrix& minv) const = 0;
```
World space transform of shape.  
* *m* - transform matrix of shape.
* *minv* - inversed *m*.
```
virtual void Shape::SetLinearVelocity(float3 const& v) = 0;
virtual float3 Shape::GetLinearVelocity() const = 0;
virtual void Shape::SetAngularVelocity(quaternion const& v) = 0;
virtual quaternion Shape::GetAngularVelocity() const = 0;
```
Linear velocity of shape for motion blur.  
* *v* - velocity.
```
virtual void Shape::SetId(Id id) = 0;
virtual Id Shape::GetId() const = 0;
```
Work with shape id. This id will be used in results of QueryIntersection().  
* *id* - new shape id.

#### Memory management
```
Buffer* IntersectionApi::CreateBuffer(size_t size, void* initdata) const override;
```
Return new buffer object.  
* *size* - buffer size in bytes.
* *initdata* - initial data for the buffer. Can be nullptr.

```
virtual void IntersectionApi::MapBuffer(Buffer* buffer, MapType type, size_t offset, size_t size, void** data, Event** event) const override;
```
Map buffer. The call is asynchronous.  
* *buffer* - the buffer to map.
* *type* - map goal. Can be kMapRead or kMapWrite.
* *offset* - offset on host memory.
* *size* - size of buffer to map.
* *data* - out mapped data on host memory.
* *event* - out event. This event can be used to track the status of execution
or to build dependency chains.
```
virtual void IntersectionApi::UnmapBuffer(Buffer* buffer, void* ptr, Event** event) const = 0;
```
Unmap buffer. The call is asynchronous.  
* *buffer* - the buffer to unmap.
* *ptr* - pointer returned by IntersectionApi::MapBuffer call.
* *event* - out event. This event can be used to track the status of execution
or to build dependency chains.
#### Events handling
Events can be created when calling asynchronous methods.
```
virtual void IntersectionApi::QueryIntersection(Buffer const* rays, int numrays, Buffer* hitinfos, Event const* waitevent, Event** event) const = 0;
```
Find closest intersection. The call is asynchronous. Event pointers might be
nullptrs.  
* *rays* -  buffer with ray data.
* *numrays* - count of rays to use from *rays* buffer.
* *hitinfos* - buffer to store intersection results.
* *waitevent* - event to wait before start QueryIntersection().
* *event* - out event to control. This event can be used to track the status of
execution or to build dependency chains.
```
virtual void IntersectionApi::QueryOcclusion(Buffer const* rays, int numrays, Buffer* hitresults, Event const* waitevent, Event** event) const = 0;
```
Find any intersection. The call is asynchronous. Event pointers might be
nullptrs.  
* *rays* -  buffer with ray data.
* *numrays* - count of rays to use from *rays* buffer.
* *hitresults* - buffer to store intersection results. Should contain int
values(1 - hit, -1 - no hit) per ray.
* *waitevent* - event to wait before start QueryIntersection().
* *event* - out event to control. This event can be used to track the status of
execution or to build dependency chains.
#### Utility
```
virtual void SetOption(char const* name, char const* value) = 0;
virtual void SetOption(char const* name, float value) = 0;
```
Options control various aspects of RadeonRays behavior. Supported options:
* option "bvh.type" values {"bvh" (regular bvh, default), "qbvh" (4 branching
factor), "hlbvh" (fast builds)}
* option "bvh.force2level" values {0(default), 1}
by default 2-level BVH is used only if there is instancing in the scene or
motion blur is enabled. 1 forces 2-level BVH for all cases.
* option "bvh.builder" values {"sah" (use surface area heuristic), "median"
(use spatial median, faster to build, default)}
* option "bvh.sah.use_splits" values {0(default),1} (allow spatial splits for BVH)
* option "bvh.sah.traversal_cost" values {float, default = 10.f for GPU } (cost
of node traversal vs triangle intersection)
* option "bvh.sah.min_overlap" values { float < 1.f, default = 0.005f }
(overlap area which is considered for a spatial splits, fraction of parent bbox)
* option "bvh.sah.max_split_depth" values {int, default = 10} (max depth in the
tree where spatial split can happen)
* option "bvh.sah.extra_node_budget" values {float, default = 1.f} (maximum node
 memory budget compared to normal bvh (2*num_tris - 1), for ex. 0.3 = 30% more
 nodes allowed
 #### OpenCL interop
 ```
 IntersectionApi* CreateFromOpenClContext(cl_context context, cl_device_id device, cl_command_queue queue);
 ```
 Create RadeonRays api from specified context.
 * *context* - desired context.
 * *device* - desired device.
 * *queue* - desired queue.
 ```
 Buffer* CreateFromOpenClBuffer(IntersectionApi* api, cl_mem buffer);
 ```
Create Buffer object from opencl buffer.
* *api* - RadeonRays api.
* *device* - desired opencl buffer.
