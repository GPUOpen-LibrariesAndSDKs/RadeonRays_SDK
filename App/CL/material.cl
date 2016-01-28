//
// Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef MATERIAL_CL
#define MATERIAL_CL

#include <../App/CL/utils.cl>
#include <../App/CL/random.cl>
#include <../App/CL/texture.cl>
#include <../App/CL/payload.cl>
#include <../App/CL/bxdf.cl>

void Material_Select(// Scene data
                     Scene const* scene,
                     // Incoming direction
                     float3 wi,
                     // Texture args
                     TEXTURE_ARG_LIST,
                     // RNG
                     Rng* rng,
                     // Geometry
                     DifferentialGeometry* dg
                   )
{
	switch(dg->mat.type)
    {
		case kFresnelBlend:
		{
			float etai = 1.f;
			float etat = dg->mat.ni;
			float cosi = dot(dg->n, wi);

			bool entering = cosi > 0.f;

			// Revert normal and eta if needed
			if (!entering)
			{
				float tmp = etai;
				etai = etat;
				etat = tmp;
				cosi = -cosi;
			}

			float eta = etai / etat;
			float sini2 = 1.f - cosi * cosi;
			float sint2 = eta * eta * sini2;

			int idx = 0;

			if (sint2 >= 1.f)
			{
				// Sample top
				idx = dg->mat.brdftopidx;
				//
				dg->mat = scene->materials[idx];
				dg->mat.fresnel = 1.f;
				break;
			}

			float cost = native_sqrt(max(0.f, 1.f - sint2));
			float F = FresnelDielectric(etai, etat, cosi, cost);
			float r = RandFloat(rng);

			if (r < F)
			{
				// Sample top
				idx = dg->mat.brdftopidx;
				//
				dg->mat = scene->materials[idx];
				dg->mat.fresnel = 1.f;
			}
			else
			{
				// Sample base
				idx = dg->mat.brdfbaseidx;
				//
				dg->mat = scene->materials[idx];
				dg->mat.fresnel = 1.f;
			}
		}
		break;
		default:
		{
			if (dg->mat.fresnel > 0.f)
			{
				float etai = 1.f;
				float etat = dg->mat.ni;
				float cosi = dot(dg->n, wi);

				bool entering = cosi > 0.f;

				// Revert normal and eta if needed
				if (!entering)
				{
					float tmp = etai;
					etai = etat;
					etat = tmp;
					cosi = -cosi;
				}

				float eta = etai / etat;
				float sini2 = 1.f - cosi * cosi;
				float sint2 = eta * eta * sini2;

				if (sint2 >= 1.f)
				{
					dg->mat.fresnel = 1.f;
					break;
				}

				float cost = native_sqrt(max(0.f, 1.f - sint2));
				dg->mat.fresnel = FresnelDielectric(etai, etat, cosi, cost);
			}
			else
			{
				dg->mat.fresnel = 1.f;
			}
		}
		break;
    }
}


#endif // MATERIAL_CL