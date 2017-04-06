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

#define PI 3.14159265358979323846

uniform vec3 CameraForward;
uniform vec3 CameraRight;
uniform vec3 CameraUp;
uniform vec2 CameraSensorSize;
uniform float CameraFocalLength;

uniform sampler2D IblTexture;
uniform float IblMultiplier;

varying vec2 Uv;

vec4 IBL(vec3 V)
{
    float T = atan2(V.x, V.z);
    float Phi = (float)((T > 0.f) ? T : (T + 2.0 * PI));
    float Theta = acos(V.y);

    vec2 UV;
    UV.x = Phi / (2 * PI);
    UV.y = Theta / PI;

    return IblMultiplier * texture(IblTexture, UV);
}

void main()
{
    vec2 ClipSample = Uv - vec2(0.5, 0.5);
    // Transform into [-dim/2, dim/2]
    vec2 SensorSample = ClipSample * CameraSensorSize;
    // Calculate direction to image plane
    vec3 V = normalize(CameraFocalLength * CameraForward + SensorSample.x * CameraRight + SensorSample.y * CameraUp);

    gl_FragData[0] = IBL(V);
}