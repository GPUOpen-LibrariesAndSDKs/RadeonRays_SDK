/**********************************************************************
Copyright ©2015 Advanced Micro Devices, Inc. All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

•   Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
•   Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
********************************************************************/
#ifndef RAY_H
#define RAY_H

#ifdef WIN32
#ifdef EXPORT_API
#define FRAPI __declspec(dllexport)
#else
#define FRAPI __declspec(dllimport)
#endif
#else
#define FRAPI
#endif

#include <limits>

#include "float3.h"
#include "float2.h"
#include "int2.h"

namespace FireRays
{
    struct ray
    {
        ray(float3 const& oo = float3(0,0,0), 
			float3 const& dd = float3(0,0,0), 
			float maxt = std::numeric_limits<float>::max(), 
			float time = 0.f)
            : o(oo)
            , d(dd)
        {
            SetMaxT(maxt);
            SetTime(time);
			SetMask(0xFFFFFFFF);
			SetActive(true);
        }

        float3 operator ()(float t) const
        {
            return o + t * d;
        }

        void SetTime(float time)
        {
            d.w = time;
        }

		float GetTime() const
		{
			return d.w;
		}

        void SetMaxT(float maxt)
        {
            o.w = maxt;
        }

		float GetMaxT() const
		{
			return o.w;
		}

		void SetMask(int mask)
		{
			extra.x = mask;
		}

		int GetMask() const
		{
			return extra.x;
		}

		void SetActive(bool active)
		{
			extra.y = active ? 1 : 0;
		}

		bool IsActive() const
		{
			return extra.y > 0;
		}

        float4 o;
        float4 d;
		int2 extra;
		int2 padding;
    };
}

#endif // RAY_H