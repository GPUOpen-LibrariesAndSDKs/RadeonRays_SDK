#ifndef MATH_UTILS_CU
#define MATH_UTILS_CU

inline __host__ __device__ float native_recip(float in)
{
    return 1.f/in;
}

inline __host__ __device__ float3 native_recip(float3 in)
{
    return make_float3( native_recip(in.x), 
                        native_recip(in.y), 
                        native_recip(in.z));
}

inline __host__ __device__ float mad(float a, float b, float c)
{
    return fmaf(a,b,c);
}

inline __host__ __device__ float3 mad(float3 a, float3 b, float3 c)
{
    return  make_float3(fmaf(a.x, b.x, c.x), 
                        fmaf(a.y, b.y, c.y), 
                        fmaf(a.z, b.z, c.z));

}

inline __host__ __device__ float3 max(float3 a, float3 b)
{
    return  make_float3(fmaxf(a.x, b.x), 
                        fmaxf(a.y, b.y), 
                        fmaxf(a.z, b.z));

}

inline __host__ __device__ float3 min(float3 a, float3 b)
{
    return  make_float3(fminf(a.x, b.x), 
                        fminf(a.y, b.y), 
                        fminf(a.z, b.z));

}

//from cuda cutil_math.h

////////////////////////////////////////////////////////////////////////////////
// dot product
////////////////////////////////////////////////////////////////////////////////

inline __host__ __device__ float dot(float2 a, float2 b)
{ 
    return a.x * b.x + a.y * b.y;
}
inline __host__ __device__ float dot(float3 a, float3 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline __host__ __device__ float dot(float4 a, float4 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline __host__ __device__ int dot(int2 a, int2 b)
{ 
    return a.x * b.x + a.y * b.y;
}
inline __host__ __device__ int dot(int3 a, int3 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline __host__ __device__ int dot(int4 a, int4 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline __host__ __device__ uint dot(uint2 a, uint2 b)
{ 
    return a.x * b.x + a.y * b.y;
}
inline __host__ __device__ uint dot(uint3 a, uint3 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
inline __host__ __device__ uint dot(uint4 a, uint4 b)
{ 
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

////////////////////////////////////////////////////////////////////////////////
// length
////////////////////////////////////////////////////////////////////////////////

inline __host__ __device__ float length(float2 v)
{
    return sqrtf(dot(v, v));
}
inline __host__ __device__ float length(float3 v)
{
    return sqrtf(dot(v, v));
}
inline __host__ __device__ float length(float4 v)
{
    return sqrtf(dot(v, v));
}

////////////////////////////////////////////////////////////////////////////////
// normalize
////////////////////////////////////////////////////////////////////////////////

inline __host__ __device__ float2 normalize(float2 v)
{
    float invLen = rsqrtf(dot(v, v));
    return v * invLen;
}
inline __host__ __device__ float3 normalize(float3 v)
{
    float invLen = rsqrtf(dot(v, v));
    return v * invLen;
}
inline __host__ __device__ float4 normalize(float4 v)
{
    float invLen = rsqrtf(dot(v, v));
    return v * invLen;
}

////////////////////////////////////////////////////////////////////////////////
// cross product
////////////////////////////////////////////////////////////////////////////////

inline __host__ __device__ float3 cross(float3 a, float3 b)
{ 
    return make_float3(a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); 
}

#endif //MATH_UTILS_CU
