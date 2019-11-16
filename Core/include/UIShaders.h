#ifndef _UIShaders_H_
#define _UIShaders_H_

/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2019 Gsage Contributors

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
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
-----------------------------------------------------------------------------
*/

#include "GsageDefinitions.h"

namespace Gsage {
  // shader programs sources that can be used by render system to display UI
  GSAGE_API static const char* UI_VS_D3D11 =
  {
    "cbuffer vertexBuffer : register(b0) \n"
    "{\n"
      "float4x4 ProjectionMatrix; \n"
    "};\n"
    "struct VS_INPUT\n"
    "{\n"
      "float2 pos : POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
      "float4 pos : SV_POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
      "PS_INPUT output;\n"
      "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
      "output.col = input.col;\n"
      "output.uv  = input.uv;\n"
      "return output;\n"
    "}"
  };

  GSAGE_API static const char* UI_PS_D3D11 =
  {
    "struct PS_INPUT\n"
    "{\n"
      "float4 pos : SV_POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler sampler0;\n"
    "Texture2D texture0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
      "float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
      "return out_col; \n"
    "}"
  };
  GSAGE_API static const char* UI_VS_D3D9 =
  {
    "uniform float4x4 ProjectionMatrix; \n"
    "struct VS_INPUT\n"
    "{\n"
      "float2 pos : POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
      "float4 pos : POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
      "PS_INPUT output;\n"
      "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
      "output.col = input.col;\n"
      "output.uv  = input.uv;\n"
      "return output;\n"
    "}"
  };

  GSAGE_API static const char* UI_PS_D3D9 =
  {
    "struct PS_INPUT\n"
    "{\n"
      "float4 pos : SV_POSITION;\n"
      "float4 col : COLOR0;\n"
      "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler2D sampler0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
      "float4 out_col = input.col.bgra * tex2D(sampler0, input.uv); \n"
      "return out_col; \n"
    "}"
  };

  GSAGE_API static const char* UI_VS_GLSL150 =
  {
    "#version 150\n"
    "uniform mat4 ProjectionMatrix; \n"
    "in vec2 vertex;\n"
    "in vec2 uv0;\n"
    "in vec4 colour;\n"
    "out vec2 Texcoord;\n"
    "out vec4 col;\n"
    "void main()\n"
    "{\n"
      "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
      "Texcoord  = uv0;\n"
      "col = colour;\n"
    "}"
  };

  GSAGE_API static const char* UI_VS_GLSL120 =
  {
    "#version 120\n"
    "uniform mat4 ProjectionMatrix; \n"
    "attribute vec2 vertex;\n"
    "attribute vec2 uv0;\n"
    "attribute vec4 colour;\n"
    "varying vec2 Texcoord;\n"
    "varying vec4 col;\n"
    "void main()\n"
    "{\n"
      "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
      "Texcoord  = uv0;\n"
      "col = colour;\n"
    "}"
  };

  GSAGE_API static const char* UI_PS_GLSL150 =
  {
    "#version 150\n"
    "in vec2 Texcoord;\n"
    "in vec4 col;\n"
    "uniform sampler2D sampler0;\n"
    "out vec4 out_col;\n"
    "void main()\n"
    "{\n"
      "out_col = col * texture(sampler0, Texcoord); \n"
    "}"
  };

  GSAGE_API static const char* UI_PS_GLSL120 =
  {
    "#version 120\n"
    "varying vec2 Texcoord;\n"
    "varying vec4 col;\n"
    "uniform sampler2D sampler0;\n"
    "void main()\n"
    "{\n"
      "gl_FragColor = col * texture2D(sampler0, Texcoord); \n"
    "}"
  };

  GSAGE_API static const char* UI_VS_METAL =
  {
    "#include <metal_stdlib> \n"
    "using namespace metal; \n"

    "struct VS_INPUT\n"
    "{\n"
      "float2 position	[[attribute(VES_POSITION)]];\n"
      "float2 uv0		[[attribute(VES_TEXTURE_COORDINATES0)]];\n"
      "float4 color		[[attribute(VES_DIFFUSE)]];\n"
    "};\n"

    "struct PS_INPUT\n"
    "{\n"
      "float2 uv0;\n"
      "float4 gl_Position [[position]];\n"
      "float4 color;\n"
    "};\n"

    "vertex PS_INPUT main_metal\n"
    "(\n"
    "VS_INPUT input [[stage_in]],\n"

    "constant float4x4 &ProjectionMatrix [[buffer(PARAMETER_SLOT)]]\n"
    ")\n"
    "{\n"
      "PS_INPUT outVs;\n"
      "outVs.gl_Position	= ( ProjectionMatrix * float4(input.position.xy, 0.f, 1.f) ).xyzw;\n"
      "outVs.uv0			= input.uv0;\n"
      "outVs.color = input.color;\n"

      "return outVs;\n"
    "}"
  };

  GSAGE_API static const char* UI_PS_METAL =
  {
    "#include <metal_stdlib>\n"
    "using namespace metal;\n"
    "struct PS_INPUT\n"
    "{\n"
      "float2 uv0;\n"
      "float4 color;\n"
    "};\n"
    "fragment float4 main_metal\n"
    "(\n"
    "PS_INPUT inPs [[stage_in]],\n"
    "texture2d<float>	tex	[[texture(0)]],\n"
    "sampler				samplerState	[[sampler(0)]]\n"
    ")\n"
    "{\n"
      "float4 col = inPs.color * tex.sample(samplerState, inPs.uv0);\n"
      "return col;\n"
    "}\n"
  };
}

#endif
