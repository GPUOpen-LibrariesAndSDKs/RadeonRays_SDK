/**********************************************************************
Copyright (c) 2020 Advanced Micro Devices, Inc. All rights reserved.
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
#ifndef RADEONRAYS_H
#define RADEONRAYS_H

#define RR_API_MAJOR_VERSION 0x000000
#define RR_API_MINOR_VERSION 0x000001
#define RR_API_PATCH_VERSION 0x000001
#define RR_API_VERSION RR_API_MAJOR_VERSION * 1000000 + RR_API_MINOR_VERSION * 1000 + RR_API_PATCH_VERSION

#ifdef _WIN32
#ifdef RR_EXPORT_API
#define RR_API __declspec(dllexport)
#else
#define RR_API __declspec(dllimport)
#endif
#else
#define RR_API __attribute__((visibility("default")))
#endif

#include <stddef.h>
#include <stdint.h>

struct _RRDevicePtr;
struct _RREvent;
struct _RRContext;
struct _RRCommandStream;

typedef uint32_t          RRBuildFlags;
typedef uint32_t          RRRayMask;
typedef _RRDevicePtr*     RRDevicePtr;
typedef _RRContext*       RRContext;
typedef _RREvent*         RREvent;
typedef _RRCommandStream* RRCommandStream;

enum
{
    RR_INAVLID_VALUE = ~0u
};

typedef enum
{
    RR_SUCCESS                    = 0,
    RR_ERROR_NOT_IMPLEMENTED      = 1,
    RR_ERROR_INTERNAL             = 2,
    RR_ERROR_OUT_OF_HOST_MEMORY   = 3,
    RR_ERROR_OUT_OF_DEVICE_MEMORY = 4,
    RR_ERROR_INVALID_API_VERSION  = 5,
    RR_ERROR_INVALID_PARAMETER    = 6,
    RR_ERROR_UNSUPPORTED_API      = 7,
    RR_ERROR_UNSUPPORTED_INTEROP  = 8
} RRError;

typedef enum
{
    RR_API_DX     = 1,
    RR_API_VK     = 2
} RRApi;

typedef enum
{
    RR_LOG_LEVEL_DEBUG = 1,
    RR_LOG_LEVEL_INFO  = 2,
    RR_LOG_LEVEL_WARN  = 3,
    RR_LOG_LEVEL_ERROR = 4,
    RR_LOG_LEVEL_OFF   = 5
} RRLogLevel;

/** @brief Type of geometry/scene build operation.
 *
 * rrBuildGeometry/rrBuildScene can either build or update
 * an underlying acceleration structure.
 */
typedef enum
{
    RR_BUILD_OPERATION_BUILD  = 1,
    RR_BUILD_OPERATION_UPDATE = 2
} RRBuildOperation;

/** @brief Hint flags for geometry/scene build functions.
 *
 * RRBuildGeometry/rtBuildScene use these flags to choose
 * an appropriate build format/algorithm.
 */
typedef enum
{
    RR_BUILD_FLAG_BITS_PREFER_FAST_BUILD = 1,
    RR_BUILD_FLAG_BITS_ALLOW_UPDATE      = 2
} RRBuildFlagBits;

/** @brief Geometric primitive type.
 *
 * RRGeometry can be built from multiple primitive types,
 * such as triangle meshes, AABB lists, line lists, etc. This enum
 * defines primitive type for RRBuildGeometry function.
 */
typedef enum
{
    RR_PRIMITIVE_TYPE_TRIANGLE_MESH,
    RR_PRIMITIVE_TYPE_AABB_LIST,
} RRPrimitiveType;

/** @brief Index type for indexed primitives.
 *
 */
typedef enum
{
    RR_INDEX_TYPE_UINT32,
    RR_INDEX_TYPE_UINT16
} RRIndexType;

/** @brief Query type for rrIntersect/rrIntersectIndirect.
 *
 */
typedef enum
{
    RR_INTERSECT_QUERY_CLOSEST = 0,
    RR_INTERSECT_QUERY_ANY     = 1
} RRIntersectQuery;

/** @brief Output type for rrIntersect
 */
typedef enum
{
    RR_INTERSECT_QUERY_OUTPUT_FULL_HIT,
    RR_INTERSECT_QUERY_OUTPUT_INSTANCE_ID
} RRIntersectQueryOutput;

/** @brief Ray description for rrIntersect
 */
typedef struct
{
    float origin[3];
    float min_t;
    float direction[3];
    float max_t;
} RRRay;

/** @brief Hit description for full hit results of rrIntersect
 */
typedef struct
{
    float    uv[2];
    uint32_t inst_id;
    uint32_t prim_id;
} RRHit;

/** @brief Various flags controlling scene/geometry build process.
 */
typedef struct
{
    RRBuildFlags build_flags;
    void*        backend_specific_info;

    // TODO: anything else here?
    // TODO: extend for TIER 2
} RRBuildOptions;

/** @brief Triangle mesh primitive.
 *
 * Triangle mesh primitive is represented as an indexed vertex aRRay.
 * Vertex and index aRRays are defined using device pointers and strides.
 * Each vertex has to have 3 components: (x, y, z) coordinates.
 * Indices are organized into triples (i0, i1, i2) - one for each triangle.
 */
typedef struct
{
    RRDevicePtr vertices;      /*!< Device pointer to vertex data */
    uint32_t    vertex_count;  /*!< Number of vertices in vertex aRRay */
    uint32_t    vertex_stride; /*!< Stride in bytes between two vertices */

    RRDevicePtr triangle_indices; /*!< Device pointer to index data */
    uint32_t    triangle_count;   /*!< Number of trinagles in index aRRay */

    RRIndexType index_type; /*!< Index type */
} RRTriangleMeshPrimitive;

/** @brief AABB list primitive.
 *
 * AABB list is an aRRay of axis aligned bounding boxes, represented
 * by device memory pointer and stride between two consequetive boxes.
 * Each AABB is a pair of float4 values (xmin, ymin, zmin, unused), (xmax, ymax,
 * zmax, unused).
 */
typedef struct
{
    RRDevicePtr aabbs;       /*!< Device pointer to AABB data */
    uint32_t    aabb_count;  /*!< Number of AABBs in the aRRay */
    uint32_t    aabb_stride; /*!< Stride in bytes between two AABBs */
} RRAABBListPrimitive;

/** @brief Input for geometry build/update operation.
 *
 * Build input defines concrete primitive type and a pointer to an actual
 * primitive description.
 */
typedef struct
{
    /*!< Defines the following union */
    RRPrimitiveType primitive_type;
    /*!< Number of primitives in the aRRay */

    uint32_t primitive_count;
    union
    {
        RRTriangleMeshPrimitive* triangle_mesh_primitives;
        RRAABBListPrimitive*     aabb_primitives;
    };

} RRGeometryBuildInput;

typedef struct
{
    RRDevicePtr geometry;
    float       transform[3][4];
} RRInstance;

/** @brief Build input for the scene.
 *
 * Scene consists of a set of instances. Each of the instances is defined by:
 *  - Root pointer of the corresponding geometry
 *  - Transformation matrix
 *  - Mask
 *
 * Instances can refer to the same geometry, but with different transformation
 * matrices (essentially implementing instancing). Mask is used to implement ray
 * masking: ray mask is bitwise &ded with an instance mask and no intersections
 * are evaluated with the primitive of corresponding instance if the result is 0.
 */
typedef struct
{
    /*!< ARRay of instanceCount pointers to instance objects */
    const RRInstance* instances;
    /*!< Number of instances in instanceGeometries */
    uint32_t instance_count;
} RRSceneBuildInput;

typedef struct
{
    size_t temporary_build_buffer_size;
    size_t temporary_update_buffer_size;
    size_t result_buffer_size;
} RRMemoryRequirements;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Create RR API context.
 *
 * All RR functions expect context as their first argument. Context
 * keeps global data required by RR session. Calls made from different
 * threads with different RR contexts are safe. Calls with the same context
 * should be externally synchronized by the client.
 *
 * @param api_version API version.
 * @param context Created context.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrCreateContext(uint32_t api_version, RRApi api, RRContext* context);

/** @brief Destory RR API context.
 *
 * Destroys all the global resources used by RR session. Further calls
 * with this context are prohibited.
 *
 * @param context API context.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrDestroyContext(RRContext context);

/** @brief Set log level
 *
 * By default all logs are shown to console
 * @param context API context.
 * @param preferred log level.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrSetLogLevel(RRLogLevel log_level);

/** @brief Set logging file
 *
 * By default all logs are shown to console
 * @param context API context.
 * @param preferred log file
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrSetLogFile(char const* filename);

/** @brief Build or update a geometry.
 *
 * Given geometry description from the client, this function builds
 * RRGeometry representing acceleration structure topology (in case of a
 * build) or updates acceleration structure keeping topology intact (update).
 *
 * @param context RR API context.
 * @param build_operation Type of build operation.
 * @param build_input Describes input primitive to build geometry from.
 * @param build_options Various flags controlling build process.
 * @param temporary_buffer Temporary buffer for build operation.
 * @param geometry_buffer Buffer to put geometry to.
 * @param command_stream Command stream to write command into.
 * @return Error in case of a failure, RRSuccess otherwise.
 */

RR_API RRError rrCmdBuildGeometry(RRContext                   context,
                                  RRBuildOperation            build_operation,
                                  const RRGeometryBuildInput* build_input,
                                  const RRBuildOptions*       build_options,
                                  RRDevicePtr                 temporary_buffer,
                                  RRDevicePtr                 geometry_buffer,
                                  RRCommandStream             command_stream);

/** @brief Get memory requirements for geometry build.
 *
 * @param context RR API context.
 * @param build_input Describes input primitive to build geometry from.
 * @param build_options Various flags controlling build process.
 * @param memory_requirements Pointer to write result to.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrGetGeometryBuildMemoryRequirements(RRContext                   context,
                                                    const RRGeometryBuildInput* build_input,
                                                    const RRBuildOptions*       build_options,
                                                    RRMemoryRequirements*       memory_requirements);

/** @brief Build or update a scene.
 *
 * Given a number of RRGeometries from the client, this function builds
 * RRScene representing top level acceleration structure topology (in case of
 * a build) or updates acceleration structure keeping topology intact (update).
 *
 * @param context RR API context.
 * @param build_input Decribes input geometires to build scene for.
 * @param build_options Various flags controlling build process.
 * @param temporary_buffer Temporary buffer for build operation.
 * @param scene_buffer Buffer to write scene to.
 * @param command_stream Command stream to write command.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrCmdBuildScene(RRContext                context,
                               const RRSceneBuildInput* build_input,
                               const RRBuildOptions*    build_options,
                               RRDevicePtr              temporary_buffer,
                               RRDevicePtr              scene_buffer,
                               RRCommandStream          command_stream);

/** @brief Get memory requirements for scene build.
 *
 * @param context RR API context.
 * @param build_input Decribes input geometires to build scene for.
 * @param build_options Various flags controlling build process.
 * @param memory_requirements Pointer to write result to.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrGetSceneBuildMemoryRequirements(RRContext                context,
                                                 const RRSceneBuildInput* build_input,
                                                 const RRBuildOptions*    build_options,
                                                 RRMemoryRequirements*    memory_requirements);

/** @brief Intersect ray buffer.
 *
 * @param context RR API context.
 * @param scene Scene to raycast against.
 * @param query Query type (clsosest or first)
 * @param rays Buffer of rays.
 * @param ray_count Number of rays in the buffer (or max number of rays if indirect_ray_count is supplied).
 * @param indirect_ray_count Optional actual number of rays in the buffer.
 * @param query_output Type of the information to output.
 * @param hits Output hits buffer.
 * @param scratch Auxilliary buffer for trace.
 * @param command_stream to write command to.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrCmdIntersect(RRContext              context,
                              RRDevicePtr            scene_buffer,
                              RRIntersectQuery       query,
                              RRDevicePtr            rays,
                              uint32_t               ray_count,
                              RRDevicePtr            indirect_ray_count,
                              RRIntersectQueryOutput query_output,
                              RRDevicePtr            hits,
                              RRDevicePtr            scratch,
                              RRCommandStream        command_stream);

/** @brief Get memory requirements for trace scracth buffer.
 *
 * @param context RR API context.
 * @param ray_count Number of rays in the buffer (or max number of rays if indirect_ray_count is supplied).
 * @param scratch_size Pointer to write result to.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrGetTraceMemoryRequirements(RRContext context, uint32_t ray_count, size_t* scratch_size);

/** @brief Allocate command stream.
 *
 * @param context RR API context.
 * @param command_stream Resulting command stream.
 * @return Error in case of a failure, RRSuccess
 * otherwise.
 */
RR_API RRError rrAllocateCommandStream(RRContext context, RRCommandStream* command_stream);

/** @brief Release command stream.
 *
 * @param context RR API context.
 * @param command_stream Command stream to release.
 * @return Error in case of a failure, RRSuccess
 * otherwise.
 */
RR_API RRError rrReleaseCommandStream(RRContext context, RRCommandStream command_stream);

/** @brief Submit command stream.
 *
 * @param context RR API context.
 * @param command_stream Command stream to execute.
 * @param wait_event Event to wait for.
 * @param out_event Event for this submission.
 * @return Error in case of a failure, RRSuccess
 * otherwise.
 */
RR_API RRError rrSumbitCommandStream(RRContext       context,
                                     RRCommandStream command_stream,
                                     RREvent         wait_event,
                                     RREvent*        out_event);

/** @brief Release an event.
 *
 * @param context RR API context.
 * @param event Event to release.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrReleaseEvent(RRContext context, RREvent event);

/** @brief Wait for an event on CPU.
 *
 * @param context RR API context.
 * @param event Event to wait for.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrWaitEvent(RRContext context, RREvent event);

/** @brief Release a device pointer.
 *
 * @param context RR API context.
 * @param ptr Device pointer to release.
 * @return Error in case of a failure, RRSuccess otherwise.
 */
RR_API RRError rrReleaseDevicePtr(RRContext context, RRDevicePtr ptr);

/** @brief Release command stream obtained via rrGetCommandStreamFrom*BackendCmdBuffer*.
 *
 * @param context API context.
 * @param command_stream Command stream to release.
 * @return Error in case of a failure, rrSuccess otherwise.
 */
RR_API RRError rrReleaseExternalCommandStream(RRContext context, RRCommandStream command_stream);

#ifdef __cplusplus
}
#endif

#endif  // RADEONRAYS_H
