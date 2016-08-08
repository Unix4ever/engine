#ifndef _OgreInteractionManager_H_
#define _OgreInteractionManager_H_
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

#include "GsageDefinitions.h"
#include "Serializable.h"
#include "EventSubscriber.h"

namespace MOC
{
  class CollisionTools;
}

namespace Gsage
{
  class Engine;
  class MouseEvent;
  class SelectEvent;
  class OgreRenderSystem;

  class OgreInteractionManager : public EventSubscriber<OgreInteractionManager>
  {
    public:
      OgreInteractionManager();
      virtual ~OgreInteractionManager();
      /**
       * State update
       * @param time Time const
       */
      virtual void update(const double& time);

      /**
       * Initialize interaction manager
       */
      void initialize(OgreRenderSystem* renderSystem, Engine* engine);
    private:
      virtual bool onMouseButton(EventDispatcher* sender, const Event& event);
      virtual bool onMouseMove(EventDispatcher* sender, const Event& event);
      void doRaycasting(const float& x, const float& y, unsigned int flags = 0xFFFF, bool select = false);

      OgreRenderSystem* mRenderSystem;
      Engine* mEngine;

      bool mContinuousRaycast;
      Ogre::Vector3 mMousePosition;

      MOC::CollisionTools* mCollisionTools;

      std::string mRolledOverObject;
  };
}

#endif
