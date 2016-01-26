/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

?   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
?   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#ifndef NORMALMAP_CL
#define NORMALMAP_CL

#include <../App/CL/utils.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>

void ApplyNormalMap(DifferentialGeometry* dg, TEXTURE_ARG_LIST)
{
    int nmapidx = dg->mat.nmapidx;
    if (nmapidx != -1)
    {
        // Now n, dpdu, dpdv is orthonormal basis
        float3 mappednormal = 2.f * Texture_Sample2D(dg->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f);
    
        // Return mapped version
        dg->n = normalize(mappednormal.z *  dg->n * 0.5f + mappednormal.x * dg->dpdu + mappednormal.y * dg->dpdv);
    }
}

void ApplyBumpMap(DifferentialGeometry* dg, TEXTURE_ARG_LIST)
{
	int nmapidx = dg->mat.nmapidx;
	if (nmapidx != -1)
	{
		// Now n, dpdu, dpdv is orthonormal basis
		float3 mappednormal = 2.f * Texture_SampleBump(dg->uv, TEXTURE_ARGS_IDX(nmapidx)) - make_float3(1.f, 1.f, 1.f);

		// Return mapped version
		dg->n = normalize(mappednormal.z * dg->n - mappednormal.x * dg->dpdu - mappednormal.y * dg->dpdv);
	}
}

#endif // NORMALMAP_CL