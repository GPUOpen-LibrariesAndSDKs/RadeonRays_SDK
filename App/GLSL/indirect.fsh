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

uniform sampler2D IndirectTexture;
uniform sampler2D DirectTexture;
uniform sampler2D PositionTexture;
uniform sampler2D ShadowTexture;

uniform int FilterRadius;

varying vec2 Uv;


float C(vec3 x1, vec3 x2, float sigma)
{
    float a = length(x1 - x2) / sigma;
    a *= a;
    return exp(-0.5 * a);
}

void main()
{
    vec3 Indirect = texture(IndirectTexture, Uv).xyz;
    vec3 Shadow = texture(ShadowTexture, Uv).xyz;
    vec3 FilteredIndirect = vec3(0.0, 0.0, 0.0);
    vec3 FilteredShadow = vec3(0.0, 0.0, 0.0);
    vec3 WorldPos = texture(PositionTexture, Uv).xyz;

    vec2 UvStep = vec2(1.0 / 400.0, 1.0 / 300.0);
    float SumI = 0.0;
    float SumS = 0.0;

    if (WorldPos.z < 1000.f)
    {
        for (int i = -FilterRadius; i <= FilterRadius; ++i)
        {
            for (int j = -FilterRadius; j <= FilterRadius; ++j)
            {
                vec2 CurrentUv = Uv + vec2(i, j) * UvStep;
                vec3 I = texture(IndirectTexture, CurrentUv).xyz;
                vec3 S = texture(ShadowTexture, CurrentUv).xyz;
                vec3 P = texture(PositionTexture, CurrentUv).xyz;
                FilteredIndirect += I * C(P, WorldPos, 5.0) * C(I, Indirect, 1.0);
                FilteredShadow += S * C(P, WorldPos, 5.0) * C(S, Shadow, 1.0);
                SumI += C(P, WorldPos, 5.0) * C(I, Indirect, 1.0);
                SumS += C(P, WorldPos, 5.0) * C(S, Shadow, 1.0);
            }
        }
    }

    FilteredIndirect /= SumI;
    FilteredShadow /= SumS;

    float i = 0.0;// (FilterRadius) / 10.0;
    gl_FragData[0] = vec4(FilteredShadow + 0.4, 1.0) * texture(DirectTexture, Uv) + (1 - i) * vec4(FilteredIndirect, 1.0);
}