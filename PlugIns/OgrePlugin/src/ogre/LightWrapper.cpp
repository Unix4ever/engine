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

#include "ogre/LightWrapper.h"
#include <OgreSceneNode.h>
#include <OgreSceneManager.h>

namespace Gsage {

  const std::string LightWrapper::TYPE = "light";

  LightWrapper::LightWrapper()
  {
    BIND_ACCESSOR("name", &LightWrapper::create, &LightWrapper::getName);
    BIND_ACCESSOR("lightType", &LightWrapper::setType, &LightWrapper::getType);
#if OGRE_VERSION < 0x020100
    BIND_ACCESSOR("position", &LightWrapper::setPosition, &LightWrapper::getPosition);
#else
    BIND_ACCESSOR("powerScale", &LightWrapper::setPowerScale, &LightWrapper::getPowerScale);
#endif
    BIND_ACCESSOR("colourSpecular", &LightWrapper::setSpecularColour, &LightWrapper::getSpecularColour);
    BIND_ACCESSOR("colourDiffuse", &LightWrapper::setDiffuseColour, &LightWrapper::getDiffuseColour);
    BIND_ACCESSOR("direction", &LightWrapper::setDirection, &LightWrapper::getDirection);
    BIND_ACCESSOR("castShadows", &LightWrapper::setCastShadows, &LightWrapper::getCastShadows);
  }

  void LightWrapper::create(const std::string& name)
  {
    mLight = mSceneManager->createLight(
#if OGRE_VERSION_MAJOR == 1
    name
#endif
    );
    mParentNode->attachObject(mLight);
  }

  const std::string& LightWrapper::getName() const
  {
    return mLight->getName();
  }

  void LightWrapper::setType(const std::string& type)
  {
    mLight->setType(mapType(type));
  }

  std::string LightWrapper::getType()
  {
    return mapType(mLight->getType());
  }

  void LightWrapper::setPosition(const Ogre::Vector3& position)
  {
#if OGRE_VERSION < 0x020100
    mLight->setPosition(position);
#else
    LOG(WARNING) << "Setting light position is deprecated";
#endif
  }

  Ogre::Vector3 LightWrapper::getPosition()
  {
#if OGRE_VERSION < 0x020100
    return mLight->getPosition();
#else
    LOG(WARNING) << "Getting light position is deprecated";
    return Ogre::Vector3(0, 0, 0);
#endif
  }

#if OGRE_VERSION >= 0x020100
  void LightWrapper::setPowerScale(const Ogre::Real& value)
  {
    mLight->setPowerScale(value);
  }

  Ogre::Real LightWrapper::getPowerScale()
  {
    return mLight->getPowerScale();
  }
#endif

  void LightWrapper::setDiffuseColour(const Ogre::ColourValue& value)
  {
    mLight->setDiffuseColour(value);
  }

  const Ogre::ColourValue& LightWrapper::getDiffuseColour() const
  {
    return mLight->getDiffuseColour();
  }

  void LightWrapper::setSpecularColour(const Ogre::ColourValue& value)
  {
    mLight->setSpecularColour(value);
  }

  const Ogre::ColourValue& LightWrapper::getSpecularColour() const
  {
    return mLight->getSpecularColour();
  }

  void LightWrapper::setDirection(const Ogre::Vector3& value)
  {
    mLight->setDirection(value);
  }

  Ogre::Vector3 LightWrapper::getDirection()
  {
    return mLight->getDirection();
  }

  void LightWrapper::setCastShadows(const bool& value)
  {
    mLight->setCastShadows(value);
  }

  bool LightWrapper::getCastShadows()
  {
    return mLight->getCastShadows();
  }

  Ogre::Light::LightTypes LightWrapper::mapType(const std::string& type)
  {
    if(type == "point")
      return Ogre::Light::LightTypes::LT_POINT;
    else if(type == "directional")
      return Ogre::Light::LightTypes::LT_DIRECTIONAL;
    else if(type == "spotlight")
      return Ogre::Light::LightTypes::LT_SPOTLIGHT;

    return Ogre::Light::LightTypes::LT_POINT;
  }

  std::string LightWrapper::mapType(const Ogre::Light::LightTypes& type)
  {
    if(type == Ogre::Light::LightTypes::LT_POINT)
      return "point";
    else if(type == Ogre::Light::LightTypes::LT_DIRECTIONAL)
      return "directional";
    else if(type == Ogre::Light::LightTypes::LT_SPOTLIGHT)
      return "spotlight";

    return "unknown";
  }
}
