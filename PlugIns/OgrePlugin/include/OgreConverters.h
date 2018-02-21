#ifndef _DataProxyConverters_H_
#define _DataProxyConverters_H_
#
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
#include <OgreCommon.h>
#include <OgreImage.h>
#include <OgreVector3.h>
#include <OgreColourValue.h>
#include <OgreQuaternion.h>
#include "DataProxy.h"
#include "RenderTarget.h"
#include "GeometryPrimitives.h"

namespace Gsage {
  TYPE_CASTER(OgreDegreeCaster, Ogre::Degree, std::string);
  TYPE_CASTER(OgreColourValueCaster, Ogre::ColourValue, std::string);
  TYPE_CASTER(OgreVector3Caster, Ogre::Vector3, std::string);
  TYPE_CASTER(OgreQuaternionCaster, Ogre::Quaternion, std::string);
  TYPE_CASTER(OgreFloatRectCaster, Ogre::FloatRect, std::string);
  TYPE_CASTER(OgrePixelFormatCaster, Ogre::PixelFormat, std::string);

  TYPE_CASTER(RenderTargetTypeCaster, RenderTarget::Type, std::string);

  static inline const Ogre::Vector3 GsageVector3ToOgreVector3(const Gsage::Vector3& vector)
  {
    return Ogre::Vector3(vector.X, vector.Y, vector.Z);
  }

  static inline const Gsage::Vector3 OgreVector3ToGsageVector3(const Ogre::Vector3& vector)
  {
    return Gsage::Vector3(vector.x, vector.y, vector.z);
  }

  static inline const Ogre::Quaternion GsageQuaternionToOgreQuaternion(const Gsage::Quaternion& quaternion)
  {
    return Ogre::Quaternion(quaternion.W, quaternion.X, quaternion.Y, quaternion.Z);
  }

  static inline const Gsage::Quaternion OgreQuaternionToGsageQuaternion(const Ogre::Quaternion& quaternion)
  {
    return Gsage::Quaternion(quaternion.x, quaternion.y, quaternion.z, quaternion.w);
  }
}

#endif
