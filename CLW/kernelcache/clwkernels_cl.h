/* This is an auto-generated file. Do not edit manually*/

static const char g_CLW_opencl[]= \
"/********************************************************************** \n"\
"Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved. \n"\
" \n"\
"Permission is hereby granted, free of charge, to any person obtaining a copy \n"\
"of this software and associated documentation files (the \"Software\"), to deal \n"\
"in the Software without restriction, including without limitation the rights \n"\
"to use, copy, modify, merge, publish, distribute, sublicense, and/or sell \n"\
"copies of the Software, and to permit persons to whom the Software is \n"\
"furnished to do so, subject to the following conditions: \n"\
" \n"\
"The above copyright notice and this permission notice shall be included in \n"\
"all copies or substantial portions of the Software. \n"\
" \n"\
"THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR \n"\
"IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, \n"\
"FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE \n"\
"AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER \n"\
"LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, \n"\
"OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN \n"\
"THE SOFTWARE. \n"\
"********************************************************************/ \n"\
" \n"\
"#pragma OPENCL EXTENSION cl_khr_local_int32_base_atomics : enable \n"\
"#pragma OPENCL EXTENSION cl_khr_global_int32_base_atomics : enable \n"\
" \n"\
" \n"\
"// --------------------- HELPERS ------------------------ \n"\
"//#define INT_MAX 0x7FFFFFFF \n"\
" \n"\
"// -------------------- MACRO -------------------------- \n"\
"// Apple OCL compiler has this by default,  \n"\
"// so embrace with #ifdef in the future \n"\
"#define DEFINE_MAKE_4(type)\\ \n"\
"    type##4 make_##type##4(type x, type y, type z, type w)\\ \n"\
"{\\ \n"\
"    type##4 res;\\ \n"\
"    res.x = x;\\ \n"\
"    res.y = y;\\ \n"\
"    res.z = z;\\ \n"\
"    res.w = w;\\ \n"\
"    return res;\\ \n"\
"} \n"\
" \n"\
"// Multitype macros to handle parallel primitives \n"\
"#define DEFINE_SAFE_LOAD_4(type)\\ \n"\
"    type##4 safe_load_##type##4(__global type##4* source, uint idx, uint sizeInTypeUnits)\\ \n"\
"{\\ \n"\
"    type##4 res = make_##type##4(0, 0, 0, 0);\\ \n"\
"    if (((idx + 1) << 2)  <= sizeInTypeUnits)\\ \n"\
"    res = source[idx];\\ \n"\
"    else\\ \n"\
"    {\\ \n"\
"    if ((idx << 2) < sizeInTypeUnits) res.x = source[idx].x;\\ \n"\
"    if ((idx << 2) + 1 < sizeInTypeUnits) res.y = source[idx].y;\\ \n"\
"    if ((idx << 2) + 2 < sizeInTypeUnits) res.z = source[idx].z;\\ \n"\
"    }\\ \n"\
"    return res;\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SAFE_STORE_4(type)\\ \n"\
"    void safe_store_##type##4(type##4 val, __global type##4* dest, uint idx, uint sizeInTypeUnits)\\ \n"\
"{\\ \n"\
"    if ((idx + 1) * 4  <= sizeInTypeUnits)\\ \n"\
"    dest[idx] = val;\\ \n"\
"    else\\ \n"\
"    {\\ \n"\
"    if (idx*4 < sizeInTypeUnits) dest[idx].x = val.x;\\ \n"\
"    if (idx*4 + 1 < sizeInTypeUnits) dest[idx].y = val.y;\\ \n"\
"    if (idx*4 + 2 < sizeInTypeUnits) dest[idx].z = val.z;\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE(type)\\ \n"\
"    void group_scan_exclusive_##type(int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    if (localId == 0)\\ \n"\
"    shmem[groupSize - 1] = 0;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE_SUM(type)\\ \n"\
"    void group_scan_exclusive_sum_##type(int localId, int groupSize, __local type* shmem, type* sum)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    *sum = shmem[groupSize - 1];\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    if (localId == 0){\\ \n"\
"    shmem[groupSize - 1] = 0;}\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
" \n"\
"#define DEFINE_GROUP_SCAN_EXCLUSIVE_PART(type)\\ \n"\
"    type group_scan_exclusive_part_##type( int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    type sum = 0;\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    if (localId == 0)\\ \n"\
"    {\\ \n"\
"    sum = shmem[groupSize - 1];\\ \n"\
"    shmem[groupSize - 1] = 0;\\ \n"\
"    }\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        type temp = shmem[(2*localId + 1)*stride-1];\\ \n"\
"        shmem[(2*localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1];\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + temp;\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"    return sum;\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE(type)\\ \n"\
"    __kernel void scan_exclusive_##type(__global type const* in_array, __global type* out_array, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    shmem[localId] = in_array[2*globalId] + in_array[2*globalId + 1];\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    out_array[2 * globalId + 1] = shmem[localId] + in_array[2*globalId];\\ \n"\
"    out_array[2 * globalId] = shmem[localId];\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_4(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\\ \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\\ \n"\
"    v2.w += v1.w;\\ \n"\
"    shmem[localId] = v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    v2.w = shmem[localId];\\ \n"\
"    type t = v1.w; v1.w = v2.w; v2.w += t;\\ \n"\
"    t = v1.y; v1.y = v1.w; v1.w += t;\\ \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t;\\ \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t;\\ \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t;\\ \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t;\\ \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_4_V1(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_##type##4##_v1(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    shmem[localId] = v1.x + v1.y + v1.z + v1.w + v2.x + v2.y + v2.z + v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    group_scan_exclusive_##type(localId, groupSize, shmem);\\ \n"\
"    type offset = shmem[localId];\\ \n"\
"    type t = v1.x; v1.x = offset; offset += t;\\ \n"\
"    t = v1.y; v1.y = offset; offset += t;\\ \n"\
"    t = v1.z; v1.z = offset; offset += t;\\ \n"\
"    t = v1.w; v1.w = offset; offset += t;\\ \n"\
"    t = v2.x; v2.x = offset; offset += t;\\ \n"\
"    t = v2.y; v2.y = offset; offset += t;\\ \n"\
"    t = v2.z; v2.z = offset; offset += t;\\ \n"\
"    v2.w = offset;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_SCAN_EXCLUSIVE_PART_4(type)\\ \n"\
"    __attribute__((reqd_work_group_size(64, 1, 1)))\\ \n"\
"    __kernel void scan_exclusive_part_##type##4(__global type##4 const* in_array, __global type##4* out_array, uint numElems, __global type* out_sums, __local type* shmem)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int localId   = get_local_id(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    int groupSize = get_local_size(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(in_array, 2*globalId, numElems);\\ \n"\
"    type##4 v2 = safe_load_##type##4(in_array, 2*globalId + 1, numElems);\\ \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y;\\ \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y;\\ \n"\
"    v2.w += v1.w;\\ \n"\
"    shmem[localId] = v2.w;\\ \n"\
"    barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    type sum = group_scan_exclusive_part_##type(localId, groupSize, shmem);\\ \n"\
"    if (localId == 0) out_sums[groupId] = sum;\\ \n"\
"    v2.w = shmem[localId];\\ \n"\
"    type t = v1.w; v1.w = v2.w; v2.w += t;\\ \n"\
"    t = v1.y; v1.y = v1.w; v1.w += t;\\ \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t;\\ \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t;\\ \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t;\\ \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t;\\ \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t;\\ \n"\
"    safe_store_##type##4(v2, out_array, 2 * globalId + 1, numElems);\\ \n"\
"    safe_store_##type##4(v1, out_array, 2 * globalId, numElems);\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_GROUP_REDUCE(type)\\ \n"\
"    void group_reduce_##type(int localId, int groupSize, __local type* shmem)\\ \n"\
"{\\ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1)\\ \n"\
"    {\\ \n"\
"    if (localId < groupSize/(2*stride))\\ \n"\
"        {\\ \n"\
"        shmem[2*(localId + 1)*stride-1] = shmem[2*(localId + 1)*stride-1] + shmem[(2*localId + 1)*stride-1];\\ \n"\
"        }\\ \n"\
"        barrier(CLK_LOCAL_MEM_FENCE);\\ \n"\
"    }\\ \n"\
"} \n"\
" \n"\
"#define DEFINE_DISTRIBUTE_PART_SUM_4(type)\\ \n"\
"    __kernel void distribute_part_sum_##type##4( __global type* in_sums, __global type##4* inout_array, uint numElems)\\ \n"\
"{\\ \n"\
"    int globalId  = get_global_id(0);\\ \n"\
"    int groupId   = get_group_id(0);\\ \n"\
"    type##4 v1 = safe_load_##type##4(inout_array, globalId, numElems);\\ \n"\
"    type    sum = in_sums[groupId >> 1];\\ \n"\
"    v1.xyzw += sum;\\ \n"\
"    safe_store_##type##4(v1, inout_array, globalId, numElems);\\ \n"\
"} \n"\
" \n"\
" \n"\
"// These are already defined in Apple OCL runtime \n"\
"#ifndef APPLE \n"\
"DEFINE_MAKE_4(int) \n"\
"DEFINE_MAKE_4(float) \n"\
"#endif \n"\
" \n"\
"DEFINE_SAFE_LOAD_4(int) \n"\
"DEFINE_SAFE_LOAD_4(float) \n"\
" \n"\
"DEFINE_SAFE_STORE_4(int) \n"\
"DEFINE_SAFE_STORE_4(float) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(int) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(uint) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(float) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE(short) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_SUM(uint) \n"\
" \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_PART(int) \n"\
"DEFINE_GROUP_SCAN_EXCLUSIVE_PART(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE(int) \n"\
"DEFINE_SCAN_EXCLUSIVE(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE_4(int) \n"\
"DEFINE_SCAN_EXCLUSIVE_4(float) \n"\
" \n"\
"DEFINE_SCAN_EXCLUSIVE_PART_4(int) \n"\
"DEFINE_SCAN_EXCLUSIVE_PART_4(float) \n"\
" \n"\
"DEFINE_DISTRIBUTE_PART_SUM_4(int) \n"\
"DEFINE_DISTRIBUTE_PART_SUM_4(float) \n"\
" \n"\
"/// Specific function for radix-sort needs \n"\
"/// Group exclusive add multiscan on 4 arrays of shorts in parallel \n"\
"/// with 4x reduction in registers \n"\
"void group_scan_short_4way(int localId, int groupSize, \n"\
"    short4 mask0, \n"\
"    short4 mask1, \n"\
"    short4 mask2, \n"\
"    short4 mask3, \n"\
"    __local short* shmem0, \n"\
"    __local short* shmem1, \n"\
"    __local short* shmem2, \n"\
"    __local short* shmem3, \n"\
"    short4* offset0, \n"\
"    short4* offset1, \n"\
"    short4* offset2, \n"\
"    short4* offset3, \n"\
"    short4* histogram) \n"\
"{ \n"\
"    short4 v1 = mask0; \n"\
"    v1.y += v1.x; v1.w += v1.z; v1.w += v1.y; \n"\
"    shmem0[localId] = v1.w; \n"\
" \n"\
"    short4 v2 = mask1; \n"\
"    v2.y += v2.x; v2.w += v2.z; v2.w += v2.y; \n"\
"    shmem1[localId] = v2.w; \n"\
" \n"\
"    short4 v3 = mask2; \n"\
"    v3.y += v3.x; v3.w += v3.z; v3.w += v3.y; \n"\
"    shmem2[localId] = v3.w; \n"\
" \n"\
"    short4 v4 = mask3; \n"\
"    v4.y += v4.x; v4.w += v4.z; v4.w += v4.y; \n"\
"    shmem3[localId] = v4.w; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            shmem0[2 * (localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1] + shmem0[(2 * localId + 1)*stride - 1]; \n"\
"            shmem1[2 * (localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1] + shmem1[(2 * localId + 1)*stride - 1]; \n"\
"            shmem2[2 * (localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1] + shmem2[(2 * localId + 1)*stride - 1]; \n"\
"            shmem3[2 * (localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1] + shmem3[(2 * localId + 1)*stride - 1]; \n"\
"        } \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    short4 total; \n"\
"    total.s0 = shmem0[groupSize - 1]; \n"\
"    total.s1 = shmem1[groupSize - 1]; \n"\
"    total.s2 = shmem2[groupSize - 1]; \n"\
"    total.s3 = shmem3[groupSize - 1]; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        shmem0[groupSize - 1] = 0; \n"\
"        shmem1[groupSize - 1] = 0; \n"\
"        shmem2[groupSize - 1] = 0; \n"\
"        shmem3[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem0[(2 * localId + 1)*stride - 1]; \n"\
"            shmem0[(2 * localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1]; \n"\
"            shmem0[2 * (localId + 1)*stride - 1] = shmem0[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem1[(2 * localId + 1)*stride - 1]; \n"\
"            shmem1[(2 * localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1]; \n"\
"            shmem1[2 * (localId + 1)*stride - 1] = shmem1[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem2[(2 * localId + 1)*stride - 1]; \n"\
"            shmem2[(2 * localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1]; \n"\
"            shmem2[2 * (localId + 1)*stride - 1] = shmem2[2 * (localId + 1)*stride - 1] + temp; \n"\
" \n"\
"            temp = shmem3[(2 * localId + 1)*stride - 1]; \n"\
"            shmem3[(2 * localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1]; \n"\
"            shmem3[2 * (localId + 1)*stride - 1] = shmem3[2 * (localId + 1)*stride - 1] + temp; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    v1.w = shmem0[localId]; \n"\
" \n"\
"    short t = v1.y; v1.y = v1.w; v1.w += t; \n"\
"    t = v1.x; v1.x = v1.y; v1.y += t; \n"\
"    t = v1.z; v1.z = v1.w; v1.w += t; \n"\
"    *offset0 = v1; \n"\
" \n"\
"    v2.w = shmem1[localId]; \n"\
" \n"\
"    t = v2.y; v2.y = v2.w; v2.w += t; \n"\
"    t = v2.x; v2.x = v2.y; v2.y += t; \n"\
"    t = v2.z; v2.z = v2.w; v2.w += t; \n"\
"    *offset1 = v2; \n"\
" \n"\
"    v3.w = shmem2[localId]; \n"\
" \n"\
"    t = v3.y; v3.y = v3.w; v3.w += t; \n"\
"    t = v3.x; v3.x = v3.y; v3.y += t; \n"\
"    t = v3.z; v3.z = v3.w; v3.w += t; \n"\
"    *offset2 = v3; \n"\
" \n"\
"    v4.w = shmem3[localId]; \n"\
" \n"\
"    t = v4.y; v4.y = v4.w; v4.w += t; \n"\
"    t = v4.x; v4.x = v4.y; v4.y += t; \n"\
"    t = v4.z; v4.z = v4.w; v4.w += t; \n"\
"    *offset3 = v4; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    *histogram = total; \n"\
"} \n"\
" \n"\
"// Calculate bool radix mask \n"\
"short4 radix_mask(int offset, uchar digit, int4 val) \n"\
"{ \n"\
"    short4 res; \n"\
"    res.x = ((val.x >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.y = ((val.y >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.z = ((val.z >> offset) & 3) == digit ? 1 : 0; \n"\
"    res.w = ((val.w >> offset) & 3) == digit ? 1 : 0; \n"\
"    return res; \n"\
"} \n"\
" \n"\
"// Choose offset based on radix mask value  \n"\
"short offset_4way(int val, int offset, short offset0, short offset1, short offset2, short offset3, short4 hist) \n"\
"{ \n"\
"    switch ((val >> offset) & 3) \n"\
"    { \n"\
"    case 0: \n"\
"        return offset0; \n"\
"    case 1: \n"\
"        return offset1 + hist.x; \n"\
"    case 2: \n"\
"        return offset2 + hist.x + hist.y; \n"\
"    case 3: \n"\
"        return offset3 + hist.x + hist.y + hist.z; \n"\
"    } \n"\
" \n"\
"    return 0; \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"// Perform group split using 2-bits pass \n"\
"void group_split_radix_2bits( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    int offset, \n"\
"    int4 val, \n"\
"    __local short* shmem, \n"\
"    int4* localOffset, \n"\
"    short4* histogram) \n"\
"{ \n"\
"    /// Pointers to radix flag arrays \n"\
"    __local short* shmem0 = shmem; \n"\
"    __local short* shmem1 = shmem0 + groupSize; \n"\
"    __local short* shmem2 = shmem1 + groupSize; \n"\
"    __local short* shmem3 = shmem2 + groupSize; \n"\
" \n"\
"    /// Radix masks for each digit \n"\
"    short4 mask0 = radix_mask(offset, 0, val); \n"\
"    short4 mask1 = radix_mask(offset, 1, val); \n"\
"    short4 mask2 = radix_mask(offset, 2, val); \n"\
"    short4 mask3 = radix_mask(offset, 3, val); \n"\
" \n"\
"    /// Resulting offsets \n"\
"    short4 offset0; \n"\
"    short4 offset1; \n"\
"    short4 offset2; \n"\
"    short4 offset3; \n"\
" \n"\
"    group_scan_short_4way(localId, groupSize, \n"\
"        mask0, mask1, mask2, mask3, \n"\
"        shmem0, shmem1, shmem2, shmem3, \n"\
"        &offset0, &offset1, &offset2, &offset3, \n"\
"        histogram); \n"\
" \n"\
"    (*localOffset).x = offset_4way(val.x, offset, offset0.x, offset1.x, offset2.x, offset3.x, *histogram); \n"\
"    (*localOffset).y = offset_4way(val.y, offset, offset0.y, offset1.y, offset2.y, offset3.y, *histogram); \n"\
"    (*localOffset).z = offset_4way(val.z, offset, offset0.z, offset1.z, offset2.z, offset3.z, *histogram); \n"\
"    (*localOffset).w = offset_4way(val.w, offset, offset0.w, offset1.w, offset2.w, offset3.w, *histogram); \n"\
"} \n"\
" \n"\
"int4 safe_load_int4_intmax(__global int4* source, uint idx, uint sizeInInts) \n"\
"{ \n"\
"    int4 res = make_int4(INT_MAX, INT_MAX, INT_MAX, INT_MAX); \n"\
"    if (((idx + 1) << 2) <= sizeInInts) \n"\
"        res = source[idx]; \n"\
"    else \n"\
"    { \n"\
"        if ((idx << 2) < sizeInInts) res.x = source[idx].x; \n"\
"        if ((idx << 2) + 1 < sizeInInts) res.y = source[idx].y; \n"\
"        if ((idx << 2) + 2 < sizeInInts) res.z = source[idx].z; \n"\
"    } \n"\
"    return res; \n"\
"} \n"\
" \n"\
"void safe_store_int(int val, __global int* dest, uint idx, uint sizeInInts) \n"\
"{ \n"\
"    if (idx < sizeInInts) \n"\
"        dest[idx] = val; \n"\
"} \n"\
" \n"\
"// Split kernel launcher \n"\
"__kernel void split4way(int bitshift, __global int4* in_array, uint numElems, __global int* out_histograms, __global int4* out_array, \n"\
"    __global int* out_local_histograms, \n"\
"    __global int4* out_debug_offset, \n"\
"    __local short* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
"    int numGroups = get_global_size(0) / groupSize; \n"\
" \n"\
"    /// Load single int4 value \n"\
"    int4 val = safe_load_int4_intmax(in_array, globalId, numElems); \n"\
" \n"\
"    int4 localOffset; \n"\
"    short4 localHistogram; \n"\
"    group_split_radix_2bits(localId, groupSize, bitshift, val, shmem, &localOffset, \n"\
"        &localHistogram); \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    __local int* sharedData = (__local int*)shmem; \n"\
"    __local int4* sharedData4 = (__local int4*)shmem; \n"\
" \n"\
"    sharedData[localOffset.x] = val.x; \n"\
"    sharedData[localOffset.y] = val.y; \n"\
"    sharedData[localOffset.z] = val.z; \n"\
"    sharedData[localOffset.w] = val.w; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    // Now store to memory \n"\
"    if (((globalId + 1) << 2) <= numElems) \n"\
"    { \n"\
"        out_array[globalId] = sharedData4[localId]; \n"\
"        out_debug_offset[globalId] = localOffset; \n"\
"    } \n"\
"    else \n"\
"    { \n"\
"        if ((globalId << 2) < numElems) out_array[globalId].x = sharedData4[localId].x; \n"\
"        if ((globalId << 2) + 1 < numElems) out_array[globalId].y = sharedData4[localId].y; \n"\
"        if ((globalId << 2) + 2 < numElems) out_array[globalId].z = sharedData4[localId].z; \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        out_histograms[groupId] = localHistogram.x; \n"\
"        out_histograms[groupId + numGroups] = localHistogram.y; \n"\
"        out_histograms[groupId + 2 * numGroups] = localHistogram.z; \n"\
"        out_histograms[groupId + 3 * numGroups] = localHistogram.w; \n"\
" \n"\
"        out_local_histograms[groupId] = 0; \n"\
"        out_local_histograms[groupId + numGroups] = localHistogram.x; \n"\
"        out_local_histograms[groupId + 2 * numGroups] = localHistogram.x + localHistogram.y; \n"\
"        out_local_histograms[groupId + 3 * numGroups] = localHistogram.x + localHistogram.y + localHistogram.z; \n"\
"    } \n"\
"} \n"\
" \n"\
"#define GROUP_SIZE 64 \n"\
"#define NUMBER_OF_BLOCKS_PER_GROUP 8 \n"\
"#define NUM_BINS 16 \n"\
" \n"\
"// The kernel computes 16 bins histogram of the 256 input elements. \n"\
"// The bin is determined by (in_array[tid] >> bitshift) & 0xF \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void BitHistogram( \n"\
"    // Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input array \n"\
"    __global int const* restrict in_array, \n"\
"    // Number of elements in input array \n"\
"    uint numelems, \n"\
"    // Output histograms in column layout \n"\
"    // [bin0_group0, bin0_group1, ... bin0_groupN, bin1_group0, bin1_group1, ... bin1_groupN, ...] \n"\
"    __global int* restrict out_histogram \n"\
"    ) \n"\
"{ \n"\
"    // Histogram storage \n"\
"    __local int histogram[NUM_BINS * GROUP_SIZE]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    /// Clear local histogram \n"\
"    for (int i = 0; i < NUM_BINS; ++i) \n"\
"    { \n"\
"        histogram[i*GROUP_SIZE + localid] = 0; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    const int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    const int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
" \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        /// Load single int4 value \n"\
"        int4 value = safe_load_int4_intmax(in_array, loadidx, numelems); \n"\
" \n"\
"        /// Handle value adding histogram bins \n"\
"        /// for all 4 elements \n"\
"        int4 bin = ((value >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.x*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.y >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.y*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.z >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.z*GROUP_SIZE + localid]); \n"\
"        //bin = ((value.w >> bitshift) & 0xF); \n"\
"        //++histogram[localid*kNumBins + bin]; \n"\
"        atom_inc(&histogram[bin.w*GROUP_SIZE + localid]); \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int sum = 0; \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        for (int i = 0; i < GROUP_SIZE; ++i) \n"\
"        { \n"\
"            sum += histogram[localid * GROUP_SIZE + i]; \n"\
"        } \n"\
" \n"\
"        out_histogram[numgroups*localid + groupid] = sum; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void ScatterKeys(// Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input keys \n"\
"    __global int4 const* restrict in_keys, \n"\
"    // Number of input keys \n"\
"    uint           numelems, \n"\
"    // Scanned histograms \n"\
"    __global int const* restrict  in_histograms, \n"\
"    // Output keys \n"\
"    __global int* restrict  out_keys \n"\
"    ) \n"\
"{ \n"\
"    // Local memory for offsets counting \n"\
"    __local int  keys[GROUP_SIZE * 4]; \n"\
"    __local int  scanned_histogram[NUM_BINS]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    __local uint* histogram = (__local uint*)keys; \n"\
" \n"\
"    int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    // Copy scanned histogram for the group to local memory for fast indexing \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        scanned_histogram[localid] = in_histograms[groupid + localid * numgroups]; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        // Load single int4 value \n"\
"        int4 localvals = safe_load_int4_intmax(in_keys, loadidx, numelems); \n"\
" \n"\
"        // Clear the histogram \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Do 2 bits per pass \n"\
"        for (int bit = 0; bit <= 2; bit += 2) \n"\
"        { \n"\
"            // Count histogram \n"\
"            int4 b = ((localvals >> bitshift) >> bit) & 0x3; \n"\
" \n"\
"            int4 p; \n"\
"            p.x = 1 << (8 * b.x); \n"\
"            p.y = 1 << (8 * b.y); \n"\
"            p.z = 1 << (8 * b.z); \n"\
"            p.w = 1 << (8 * b.w); \n"\
" \n"\
"            // Pack the histogram \n"\
"            uint packed_key = (uint)(p.x + p.y + p.z + p.w); \n"\
" \n"\
"            // Put into LDS \n"\
"            histogram[localid] = packed_key; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan the histogram in LDS with 4-way plus scan \n"\
"            uint total = 0; \n"\
"            group_scan_exclusive_sum_uint(localid, GROUP_SIZE, histogram, &total); \n"\
" \n"\
"            // Load value back \n"\
"            packed_key = histogram[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan total histogram (4 chars) \n"\
"            total = (total << 8) + (total << 16) + (total << 24); \n"\
"            uint offset = total + packed_key; \n"\
" \n"\
"            int4 newoffset; \n"\
" \n"\
"            int t = p.y + p.x; \n"\
"            p.w = p.z + t; \n"\
"            p.z = t; \n"\
"            p.y = p.x; \n"\
"            p.x = 0; \n"\
" \n"\
"            p += (int)offset; \n"\
"            newoffset = (p >> (b * 8)) & 0xFF; \n"\
" \n"\
"            keys[newoffset.x] = localvals.x; \n"\
"            keys[newoffset.y] = localvals.y; \n"\
"            keys[newoffset.z] = localvals.z; \n"\
"            keys[newoffset.w] = localvals.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localvals.x = keys[localid << 2]; \n"\
"            localvals.y = keys[(localid << 2) + 1]; \n"\
"            localvals.z = keys[(localid << 2) + 2]; \n"\
"            localvals.w = keys[(localid << 2) + 3]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Clear LDS \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Reconstruct 16 bins histogram \n"\
"        int4 bin = (localvals >> bitshift) & 0xF; \n"\
"        atom_inc(&histogram[bin.x]); \n"\
"        atom_inc(&histogram[bin.y]); \n"\
"        atom_inc(&histogram[bin.z]); \n"\
"        atom_inc(&histogram[bin.w]); \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        int sum = 0; \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            sum = histogram[localid]; \n"\
"        } \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Scan reconstructed histogram \n"\
"        group_scan_exclusive_uint(localid, 16, histogram); \n"\
" \n"\
"        // Put data back to global memory \n"\
"        int offset = scanned_histogram[bin.x] + (localid << 2) - histogram[bin.x]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.x; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.y] + (localid << 2) + 1 - histogram[bin.y]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.y; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.z] + (localid << 2) + 2 - histogram[bin.z]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.z; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.w] + (localid << 2) + 3 - histogram[bin.w]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localvals.w; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            scanned_histogram[localid] += sum; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
" \n"\
"__kernel \n"\
"__attribute__((reqd_work_group_size(GROUP_SIZE, 1, 1))) \n"\
"void ScatterKeysAndValues(// Number of bits to shift \n"\
"    int bitshift, \n"\
"    // Input keys \n"\
"    __global int4 const* restrict in_keys, \n"\
"    // Input values \n"\
"    __global int4 const* restrict in_values, \n"\
"    // Number of input keys \n"\
"    uint           numelems, \n"\
"    // Scanned histograms \n"\
"    __global int const* restrict  in_histograms, \n"\
"    // Output keys \n"\
"    __global int* restrict  out_keys, \n"\
"    // Output values \n"\
"    __global int* restrict  out_values \n"\
"    ) \n"\
"{ \n"\
"    // Local memory for offsets counting \n"\
"    __local int  keys[GROUP_SIZE * 4]; \n"\
"    __local int  scanned_histogram[NUM_BINS]; \n"\
" \n"\
"    int globalid = get_global_id(0); \n"\
"    int localid = get_local_id(0); \n"\
"    int groupsize = get_local_size(0); \n"\
"    int groupid = get_group_id(0); \n"\
"    int numgroups = get_global_size(0) / groupsize; \n"\
" \n"\
"    __local uint* histogram = (__local uint*)keys; \n"\
" \n"\
"    int numblocks_per_group = NUMBER_OF_BLOCKS_PER_GROUP; \n"\
"    int numelems_per_group = numblocks_per_group * GROUP_SIZE; \n"\
"    int numblocks_total = (numelems + GROUP_SIZE * 4 - 1) / (GROUP_SIZE * 4); \n"\
"    int maxblocks = numblocks_total - groupid * numblocks_per_group; \n"\
" \n"\
"    // Copy scanned histogram for the group to local memory for fast indexing \n"\
"    if (localid < NUM_BINS) \n"\
"    { \n"\
"        scanned_histogram[localid] = in_histograms[groupid + localid * numgroups]; \n"\
"    } \n"\
" \n"\
"    // Make sure everything is up to date \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    int loadidx = groupid * numelems_per_group + localid; \n"\
"    for (int block = 0; block < min(numblocks_per_group, maxblocks); ++block, loadidx += GROUP_SIZE) \n"\
"    { \n"\
"        // Load single int4 value \n"\
"        int4 localkeys = safe_load_int4_intmax(in_keys, loadidx, numelems); \n"\
"        int4 localvals = safe_load_int4_intmax(in_values, loadidx, numelems); \n"\
" \n"\
"        // Clear the histogram \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Do 2 bits per pass \n"\
"        for (int bit = 0; bit <= 2; bit += 2) \n"\
"        { \n"\
"            // Count histogram \n"\
"            int4 b = ((localkeys >> bitshift) >> bit) & 0x3; \n"\
" \n"\
"            int4 p; \n"\
"            p.x = 1 << (8 * b.x); \n"\
"            p.y = 1 << (8 * b.y); \n"\
"            p.z = 1 << (8 * b.z); \n"\
"            p.w = 1 << (8 * b.w); \n"\
" \n"\
"            // Pack the histogram \n"\
"            uint packed_key = (uint)(p.x + p.y + p.z + p.w); \n"\
" \n"\
"            // Put into LDS \n"\
"            histogram[localid] = packed_key; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan the histogram in LDS with 4-way plus scan \n"\
"            uint total = 0; \n"\
"            group_scan_exclusive_sum_uint(localid, GROUP_SIZE, histogram, &total); \n"\
" \n"\
"            // Load value back \n"\
"            packed_key = histogram[localid]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Scan total histogram (4 chars) \n"\
"            total = (total << 8) + (total << 16) + (total << 24); \n"\
"            uint offset = total + packed_key; \n"\
" \n"\
"            int4 newoffset; \n"\
" \n"\
"            int t = p.y + p.x; \n"\
"            p.w = p.z + t; \n"\
"            p.z = t; \n"\
"            p.y = p.x; \n"\
"            p.x = 0; \n"\
" \n"\
"            p += (int)offset; \n"\
"            newoffset = (p >> (b * 8)) & 0xFF; \n"\
" \n"\
"            keys[newoffset.x] = localkeys.x; \n"\
"            keys[newoffset.y] = localkeys.y; \n"\
"            keys[newoffset.z] = localkeys.z; \n"\
"            keys[newoffset.w] = localkeys.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localkeys.x = keys[localid << 2]; \n"\
"            localkeys.y = keys[(localid << 2) + 1]; \n"\
"            localkeys.z = keys[(localid << 2) + 2]; \n"\
"            localkeys.w = keys[(localid << 2) + 3]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            keys[newoffset.x] = localvals.x; \n"\
"            keys[newoffset.y] = localvals.y; \n"\
"            keys[newoffset.z] = localvals.z; \n"\
"            keys[newoffset.w] = localvals.w; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"            // Reload values back to registers for the second bit pass \n"\
"            localvals.x = keys[localid << 2]; \n"\
"            localvals.y = keys[(localid << 2) + 1]; \n"\
"            localvals.z = keys[(localid << 2) + 2]; \n"\
"            localvals.w = keys[(localid << 2) + 3]; \n"\
" \n"\
"            // Make sure everything is up to date \n"\
"            barrier(CLK_LOCAL_MEM_FENCE); \n"\
"        } \n"\
" \n"\
"        // Clear LDS \n"\
"        histogram[localid] = 0; \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Reconstruct 16 bins histogram \n"\
"        int4 bin = (localkeys >> bitshift) & 0xF; \n"\
"        atom_inc(&histogram[bin.x]); \n"\
"        atom_inc(&histogram[bin.y]); \n"\
"        atom_inc(&histogram[bin.z]); \n"\
"        atom_inc(&histogram[bin.w]); \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        int sum = 0; \n"\
"        if (localid < NUM_BINS) \n"\
"        { \n"\
"            sum = histogram[localid]; \n"\
"        } \n"\
" \n"\
"        // Make sure everything is up to date \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        // Scan reconstructed histogram \n"\
"        group_scan_exclusive_uint(localid, 16, histogram); \n"\
" \n"\
"        // Put data back to global memory \n"\
"        int offset = scanned_histogram[bin.x] + (localid << 2) - histogram[bin.x]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.x; \n"\
"            out_values[offset] = localvals.x; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.y] + (localid << 2) + 1 - histogram[bin.y]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.y; \n"\
"            out_values[offset] = localvals.y; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.z] + (localid << 2) + 2 - histogram[bin.z]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.z; \n"\
"            out_values[offset] = localvals.z; \n"\
"        } \n"\
" \n"\
"        offset = scanned_histogram[bin.w] + (localid << 2) + 3 - histogram[bin.w]; \n"\
"        if (offset < numelems) \n"\
"        { \n"\
"            out_keys[offset] = localkeys.w; \n"\
"            out_values[offset] = localvals.w; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"        scanned_histogram[localid] += sum; \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void compact_int(__global int* in_predicate, __global int* in_address, \n"\
"    __global int* in_input, uint in_size, \n"\
"    __global int* out_output) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    if (global_id < in_size) \n"\
"    { \n"\
"        if (in_predicate[global_id]) \n"\
"        { \n"\
"            out_output[in_address[global_id]] = in_input[global_id]; \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void compact_int_1(__global int* in_predicate, __global int* in_address, \n"\
"    __global int* in_input, uint in_size, \n"\
"    __global int* out_output, \n"\
"    __global int* out_size) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int group_id = get_group_id(0); \n"\
" \n"\
"    if (global_id < in_size) \n"\
"    { \n"\
"        if (in_predicate[global_id]) \n"\
"        { \n"\
"            out_output[in_address[global_id]] = in_input[global_id]; \n"\
"        } \n"\
"    } \n"\
" \n"\
"    if (global_id == 0) \n"\
"    { \n"\
"        *out_size = in_address[in_size - 1] + in_predicate[in_size - 1]; \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void copy(__global int4* in_input, \n"\
"    uint  in_size, \n"\
"    __global int4* out_output) \n"\
"{ \n"\
"    int global_id = get_global_id(0); \n"\
"    int4 value = safe_load_int4(in_input, global_id, in_size); \n"\
"    safe_store_int4(value, out_output, global_id, in_size); \n"\
"} \n"\
" \n"\
" \n"\
"#define FLAG(x) (flags[(x)] & 0x1) \n"\
"#define FLAG_COMBINED(x) (flags[(x)]) \n"\
"#define FLAG_ORIG(x) ((flags[(x)] >> 1) & 0x1) \n"\
" \n"\
"void group_segmented_scan_exclusive_int( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"        shmem[groupSize - 1] = 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            // optimize with a conditional = operator \n"\
"            if (FLAG_ORIG((2 * localId + 1)*stride) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = 0; \n"\
"            } \n"\
"            else if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
"void group_segmented_scan_exclusive_int_nocut( \n"\
"    int localId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"        shmem[groupSize - 1] = 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"void group_segmented_scan_exclusive_int_part( \n"\
"    int localId, \n"\
"    int groupId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags, \n"\
"    __global int* part_sums, \n"\
"    __global int* part_flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        part_sums[groupId] = shmem[groupSize - 1]; \n"\
"        part_flags[groupId] = FLAG(groupSize - 1); \n"\
"        shmem[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            // optimize with a conditional = operator \n"\
"            if (FLAG_ORIG((2 * localId + 1)*stride) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = 0; \n"\
"            } \n"\
"            else if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
"void group_segmented_scan_exclusive_int_nocut_part( \n"\
"    int localId, \n"\
"    int groupId, \n"\
"    int groupSize, \n"\
"    __local int* shmem, \n"\
"    __local char* flags, \n"\
"    __global int* part_sums, \n"\
"    __global int* part_flags \n"\
"    ) \n"\
"{ \n"\
"    for (int stride = 1; stride <= (groupSize >> 1); stride <<= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            if (FLAG(2 * (localId + 1)*stride - 1) == 0) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + shmem[(2 * localId + 1)*stride - 1]; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED(2 * (localId + 1)*stride - 1) = FLAG_COMBINED(2 * (localId + 1)*stride - 1) | FLAG((2 * localId + 1)*stride - 1); \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        part_sums[groupId] = shmem[groupSize - 1]; \n"\
"        part_flags[groupId] = FLAG(groupSize - 1); \n"\
"        shmem[groupSize - 1] = 0; \n"\
"    } \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    for (int stride = (groupSize >> 1); stride > 0; stride >>= 1) \n"\
"    { \n"\
"        if (localId < groupSize / (2 * stride)) \n"\
"        { \n"\
"            int temp = shmem[(2 * localId + 1)*stride - 1]; \n"\
"            shmem[(2 * localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1]; \n"\
" \n"\
"            if (FLAG((2 * localId + 1)*stride - 1) == 1) \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = temp; \n"\
"            } \n"\
"            else \n"\
"            { \n"\
"                shmem[2 * (localId + 1)*stride - 1] = shmem[2 * (localId + 1)*stride - 1] + temp; \n"\
"            } \n"\
" \n"\
"            FLAG_COMBINED((2 * localId + 1)*stride - 1) = FLAG_COMBINED((2 * localId + 1)*stride - 1) & 2; \n"\
"        } \n"\
" \n"\
"        barrier(CLK_LOCAL_MEM_FENCE); \n"\
"    } \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_nocut(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_nocut(localId, groupSize, keys, flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int(localId, groupSize, keys, flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_part(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __global int* out_part_sums, \n"\
"    __global int* out_part_flags, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_part(localId, groupId, groupSize, keys, flags, out_part_sums, out_part_flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
"__kernel void segmented_scan_exclusive_int_nocut_part(__global int const* in_array, \n"\
"    __global int const* in_segment_heads_array, \n"\
"    int numelems, \n"\
"    __global int* out_array, \n"\
"    __global int* out_part_sums, \n"\
"    __global int* out_part_flags, \n"\
"    __local int* shmem) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    __local int* keys = shmem; \n"\
"    __local char* flags = (__local char*)(keys + groupSize); \n"\
" \n"\
"    keys[localId] = globalId < numelems ? in_array[globalId] : 0; \n"\
"    flags[localId] = globalId < numelems ? (in_segment_heads_array[globalId] ? 3 : 0) : 0; \n"\
" \n"\
"    barrier(CLK_LOCAL_MEM_FENCE); \n"\
" \n"\
"    group_segmented_scan_exclusive_int_nocut_part(localId, groupId, groupSize, keys, flags, out_part_sums, out_part_flags); \n"\
" \n"\
"    out_array[globalId] = keys[localId]; \n"\
"} \n"\
" \n"\
" \n"\
"__kernel void segmented_distribute_part_sum_int( \n"\
"    __global int* inout_array, \n"\
"    __global int* in_flags, \n"\
"    int numelems, \n"\
"    __global int* in_sums \n"\
"    ) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    int sum = in_sums[groupId]; \n"\
"    //inout_array[globalId] += sum; \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        for (int i = 0; in_flags[globalId + i] == 0 && i < groupSize; ++i) \n"\
"        { \n"\
"            if (globalId + i < numelems) \n"\
"            { \n"\
"                inout_array[globalId + i] += sum; \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
" \n"\
"__kernel void segmented_distribute_part_sum_int_nocut( \n"\
"    __global int* inout_array, \n"\
"    __global int* in_flags, \n"\
"    int numelems, \n"\
"    __global int* in_sums \n"\
"    ) \n"\
"{ \n"\
"    int globalId = get_global_id(0); \n"\
"    int localId = get_local_id(0); \n"\
"    int groupSize = get_local_size(0); \n"\
"    int groupId = get_group_id(0); \n"\
" \n"\
"    int sum = in_sums[groupId]; \n"\
"    bool stop = false; \n"\
"    //inout_array[globalId] += sum; \n"\
" \n"\
"    if (localId == 0) \n"\
"    { \n"\
"        for (int i = 0; i < groupSize; ++i) \n"\
"        { \n"\
"            if (globalId + i < numelems) \n"\
"            { \n"\
"                if (in_flags[globalId + i] == 0) \n"\
"                { \n"\
"                    inout_array[globalId + i] += sum; \n"\
"                } \n"\
"                else \n"\
"                { \n"\
"                    if (stop) \n"\
"                    { \n"\
"                        break; \n"\
"                    } \n"\
"                    else \n"\
"                    { \n"\
"                        inout_array[globalId + i] += sum; \n"\
"                        stop = true; \n"\
"                    } \n"\
"                } \n"\
"            } \n"\
"        } \n"\
"    } \n"\
"} \n"\
;
