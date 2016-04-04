#ifndef PATH_CL
#define PATH_CL

#include <../App/CL/payload.cl>

typedef struct _Path
{
	float3 throughput;
	int volume;
	int flags;
	int active;
	int extra1;
} Path;

bool Path_IsScattered(__global Path const* path)
{
	return path->flags & kScattered;
}

bool Path_IsAlive(__global Path const* path)
{
	return ((path->flags & kKilled) == 0);
}

void Path_ClearScatterFlag(__global Path* path)
{
	path->flags &= ~kScattered;
}

void Path_SetScatterFlag(__global Path* path)
{
	path->flags |= kScattered;
}

void Path_Restart(__global Path* path)
{
	path->flags = 0;
}

int Path_GetVolumeIdx(__global Path const* path)
{
	return path->volume;
}

float3 Path_GetThroughput(__global Path const* path)
{
    float3 t = path->throughput;
    return t;
}

void Path_MulThroughput(__global Path* path, float3 mul)
{
	path->throughput *= mul;
}

void Path_Kill(__global Path* path)
{
	path->flags |= kKilled;
}

void Path_AddContribution(__global Path* path, __global float3* output, int idx, float3 val)
{
	output[idx] += Path_GetThroughput(path) * val;
}



#endif