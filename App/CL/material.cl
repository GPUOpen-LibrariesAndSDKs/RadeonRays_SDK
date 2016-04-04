/**********************************************************************
 Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.
 
 Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 
 ï   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 ï   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
 other materials provided with the distribution.
 
 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
 DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ********************************************************************/
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
                     // Sample
                     float sample,
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

			if (sample < F)
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
		case kMix:
		{
			if (sample < dg->mat.ns)
			{
				// Sample top
				int idx = dg->mat.brdftopidx;
				//
				dg->mat = scene->materials[idx];
				dg->mat.fresnel = 1.f;
			}
			else
			{
				// Sample base
				int idx = dg->mat.brdfbaseidx;
				//
				dg->mat = scene->materials[idx];
				dg->mat.fresnel = 1.f;
			}
			break; 
		}
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