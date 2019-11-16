/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2016 Artem Chernyshev

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

#ifndef __GsageDefinitions_H__
#define __GsageDefinitions_H__

#define ENTITY_POOL_SIZE 1024
#define COMPONENT_POOL_SIZE 1024
#define NOMINMAX

#include <sstream>
#include <istream>
#include <iterator>
#include <ostream>
#include <vector>
#include <channel>
#include <map>

static inline std::string& ltrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(0, str.find_first_not_of(chars));
    return str;
}

static inline std::string& rtrim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    str.erase(str.find_last_not_of(chars) + 1);
    return str;
}

static inline std::string& trim(std::string& str, const std::string& chars = "\t\n\v\f\r ")
{
    return ltrim(rtrim(str, chars), chars);
}

static inline std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}


static inline std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

static inline std::string join(const std::vector<std::string>& vector, const char delim)
{
  std::stringstream s;
  for(unsigned int i = 0; i < vector.size(); i++)
  {
    s << vector[i];
    if(i < vector.size() - 1)
    {
      s << delim;
    }
  }
  return s.str();
}

/**
 * Short definition of std::map key search
 */
template<class K, class T>
static bool contains(const std::map<K, T>& m, K value)
{
  return m.find(value) != m.end();
}

/**
 * Short definition of std::map key search
 */
template<class K, class T, class CompStr>
static bool contains(const std::map<K, T, CompStr>& m, K value)
{
  return m.find(value) != m.end();
}

/**
 * Short definition of std::vector key search
 */
template<class T>
static bool contains(const std::vector<T>& v, T value)
{
  return std::find(v.start(), v.end(), value) != v.end();
}

#define GSAGE_ASSERT(condition, message) assert(condition && message)

namespace Gsage {
  enum CullingMode
  {
    CULL_NONE = 0,
    CULL_CLOCKWISE,
    CULL_ANTICLOCKWISE,
    CULL_COUNT
  };

  enum TextureFilterOptions
  {
    TFO_NONE = 0,
    TFO_BILINEAR,
    TFO_TRILINEAR,
    TFO_ANISOTROPIC,
    TFO_COUNT
  };

  enum SceneBlendFactor {
    SBF_ONE = 0, SBF_ZERO, SBF_DEST_COLOUR, SBF_SOURCE_COLOUR,
    SBF_ONE_MINUS_DEST_COLOUR, SBF_ONE_MINUS_SOURCE_COLOUR, SBF_DEST_ALPHA, SBF_SOURCE_ALPHA,
    SBF_ONE_MINUS_DEST_ALPHA, SBF_ONE_MINUS_SOURCE_ALPHA, SBF_COUNT
  };

  enum SceneBlendOperation {
    SBO_ADD = 0, SBO_SUBTRACT, SBO_REVERSE_SUBTRACT, SBO_MIN,
    SBO_MAX, SBO_COUNT
  };

  enum SceneBlendType {
    SBT_TRANSPARENT_ALPHA = 0, SBT_TRANSPARENT_COLOUR, SBT_ADD, SBT_MODULATE,
    SBT_REPLACE, SBT_COUNT
  };

  enum ChannelSignal {
    NONE = 0,
    DONE = 1,
    SHUTDOWN = 2,
    FAILURE = 3
  };

  enum VertexElementSemantic {
    VES_POSITION = 1,
    VES_BLEND_WEIGHTS = 2,
    VES_BLEND_INDICES = 3,
    VES_NORMAL = 4,
    VES_DIFFUSE = 5,
    VES_SPECULAR = 6,
    VES_TEXTURE_COORDINATES = 7,
    VES_BINORMAL = 8,
    VES_TANGENT = 9
  };

  enum VertexElementType {
    VET_FLOAT1 = 0,
    VET_FLOAT2 = 1,
    VET_FLOAT3 = 2,
    VET_FLOAT4 = 3,
    VET_COLOUR = 4,
    VET_SHORT1 = 5,
    VET_SHORT2 = 6,
    VET_SHORT3 = 7,
    VET_SHORT4 = 8,
    VET_UBYTE4 = 9,
    VET_COLOUR_ARGB = 10,
    VET_COLOUR_ABGR = 11
  };

  enum PixelFormat {
    PF_UNKNOWN = 0,
    PF_L8,
    PF_L16,
    PF_A8,
    PF_BYTE_LA,
    PF_R5G6B5,
    PF_B5G6R5,
    PF_A4R4G4B4,
    PF_A1R5G5B5,
    PF_R8G8B8,
    PF_B8G8R8,
    PF_A8R8G8B8,
    PF_A8B8G8R8,
    PF_B8G8R8A8,
    PF_A2R10G10B10,
    PF_A2B10G10R10,
    PF_DXT1,
    PF_DXT2,
    PF_DXT3,
    PF_DXT4,
    PF_DXT5,
    PF_FLOAT16_RGB,
    PF_FLOAT16_RGBA,
    PF_FLOAT32_RGB,
    PF_FLOAT32_RGBA,
    PF_X8R8G8B8,
    PF_X8B8G8R8,
    PF_R8G8B8A8,
    PF_DEPTH16,
    PF_SHORT_RGBA,
    PF_R3G3B2,
    PF_FLOAT16_R,
    PF_FLOAT32_R,
    PF_SHORT_GR,
    PF_FLOAT16_GR,
    PF_FLOAT32_GR,
    PF_SHORT_RGB,
    PF_PVRTC_RGB2,
    PF_PVRTC_RGBA2,
    PF_PVRTC_RGB4,
    PF_PVRTC_RGBA4,
    PF_PVRTC2_2BPP,
    PF_PVRTC2_4BPP,
    PF_R11G11B10_FLOAT,
    PF_R8_UINT,
    PF_R8G8_UINT,
    PF_R8G8B8_UINT,
    PF_R8G8B8A8_UINT,
    PF_R16_UINT,
    PF_R16G16_UINT,
    PF_R16G16B16_UINT,
    PF_R16G16B16A16_UINT,
    PF_R32_UINT,
    PF_R32G32_UINT,
    PF_R32G32B32_UINT,
    PF_R32G32B32A32_UINT,
    PF_R8_SINT,
    PF_R8G8_SINT,
    PF_R8G8B8_SINT,
    PF_R8G8B8A8_SINT,
    PF_R16_SINT,
    PF_R16G16_SINT,
    PF_R16G16B16_SINT,
    PF_R16G16B16A16_SINT,
    PF_R32_SINT,
    PF_R32G32_SINT,
    PF_R32G32B32_SINT,
    PF_R32G32B32A32_SINT,
    PF_R9G9B9E5_SHAREDEXP,
    PF_BC4_UNORM,
    PF_BC4_SNORM,
    PF_BC5_UNORM,
    PF_BC5_SNORM,
    PF_BC6H_UF16,
    PF_BC6H_SF16,
    PF_BC7_UNORM,
    PF_R8,
    PF_RG8,
    PF_R8_SNORM,
    PF_R8G8_SNORM,
    PF_R8G8B8_SNORM,
    PF_R8G8B8A8_SNORM,
    PF_R16_SNORM,
    PF_R16G16_SNORM,
    PF_R16G16B16_SNORM,
    PF_R16G16B16A16_SNORM,
    PF_ETC1_RGB8,
    PF_ETC2_RGB8,
    PF_ETC2_RGBA8,
    PF_ETC2_RGB8A1,
    PF_ATC_RGB,
    PF_ATC_RGBA_EXPLICIT_ALPHA,
    PF_ATC_RGBA_INTERPOLATED_ALPHA,
    PF_ASTC_RGBA_4X4_LDR,
    PF_ASTC_RGBA_5X4_LDR,
    PF_ASTC_RGBA_5X5_LDR,
    PF_ASTC_RGBA_6X5_LDR,
    PF_ASTC_RGBA_6X6_LDR,
    PF_ASTC_RGBA_8X5_LDR,
    PF_ASTC_RGBA_8X6_LDR,
    PF_ASTC_RGBA_8X8_LDR,
    PF_ASTC_RGBA_10X5_LDR,
    PF_ASTC_RGBA_10X6_LDR,
    PF_ASTC_RGBA_10X8_LDR,
    PF_ASTC_RGBA_10X10_LDR,
    PF_ASTC_RGBA_12X10_LDR,
    PF_ASTC_RGBA_12X12_LDR,
    PF_DEPTH32,
    PF_COUNT,
    PF_BYTE_RGB = PF_B8G8R8,
    PF_BYTE_BGR = PF_R8G8B8,
    PF_BYTE_BGRA = PF_A8R8G8B8,
    PF_BYTE_RGBA = PF_A8B8G8R8,
    PF_BYTE_L = PF_L8,
    PF_DEPTH = PF_DEPTH16,
    PF_SHORT_L = PF_L16,
    PF_BYTE_A = PF_A8
  };

  typedef cpp::channel<ChannelSignal, 32> SignalChannel;
}

#define GSAGE_UNSUPPORTED 0
#define GSAGE_WIN32 1
#define GSAGE_LINUX 2
#define GSAGE_APPLE 3
#define GSAGE_IOS 4
#define GSAGE_ANDROID 5
#define GSAGE_UNIX 6
#define GSAGE_POSIX 7

#ifdef _WIN32
  #define GSAGE_PLATFORM GSAGE_WIN32
#elif __APPLE__
  #include "TargetConditionals.h"
  #if TARGET_IPHONE_SIMULATOR
    #define GSAGE_PLATFORM GSAGE_IOS
  #elif TARGET_OS_IPHONE
    #define GSAGE_PLATFORM GSAGE_IOS
  #elif TARGET_OS_MAC
    #define GSAGE_PLATFORM GSAGE_APPLE
  #else
    #define GSAGE_PLATFORM GSAGE_UNSUPPORTED
  #endif
#elif __ANDROID__
  #define GSAGE_PLATFORM GSAGE_ANDROID
#elif __linux
  #define GSAGE_PLATFORM GSAGE_LINUX
#elif __unix // all unices not caught above
  #define GSAGE_PLATFORM GSAGE_UNIX
#elif __posix
  #define GSAGE_PLATFORM GSAGE_POSIX
#endif

#endif

#if GSAGE_PLATFORM == GSAGE_WIN32
#define GSAGE_PATH_SEPARATOR '\\'
#define ELPP_AS_DLL
#ifdef GSAGE_DLL_EXPORT
#define GSAGE_API __declspec(dllexport)
#else
#define GSAGE_API __declspec(dllimport)
#endif
#else
#define GSAGE_PATH_SEPARATOR '/'
#define GSAGE_API
#endif
