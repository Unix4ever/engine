/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2018 Artem Chernyshev

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

#ifndef _UIManager_H_
#define _UIManager_H_

#include <string>
#include "GsageDefinitions.h"
#include "systems/RenderSystem.h"
#include "EventSubscriber.h"
#include "KeyboardEvent.h"
#include "MouseEvent.h"

struct lua_State;

namespace Gsage
{
  class GsageFacade;
  class Engine;
  class LuaInterface;

  /**
   * Single UIContext is mapped to a single render target
   */
  class GSAGE_API UIContext
  {
    public:
      UIContext()
        : mMouseX(0.0f)
        , mMouseY(0.0f)
        , mDisplaySizeX(1.0f)
        , mDisplaySizeY(1.0f)
        , mName(NULL)
      {
      }

      virtual ~UIContext()
      {
        if(mName) {
          free(mName);
        }
      }

      /**
       * Updates context
       */
      virtual void newFrame(float delta) = 0;

      /**
       * Renders context
       */
      virtual void render() = 0;

      /**
       * Resize context
       *
       * @param width display size x
       * @param height display size y
       */
      inline void resize(float width, float height)
      {
        mDisplaySizeX = width;
        mDisplaySizeY = height;
      }

      /**
       * Update mouse position
       *
       * @param x
       * @param y
       */
      inline void updateMousePos(float x, float y)
      {
        mMouseX = x;
        mMouseY = y;
      }

      /**
       * Sets current renderer
       */
      virtual void setRenderer(UIRendererPtr renderer)
      {
        mRenderer = renderer;
      }

    protected:
      friend UIManager;

      virtual bool captureMouse() const;

      virtual bool captureKey() const;

      virtual bool captureInput() const;

      virtual bool processKeyEvent(EventDispatcher* dispatcher, const KeyboardEvent& event);

      virtual bool processMouseEvent(EventDispatcher* dispatcher, const MouseEvent& event);

      virtual bool processInputEvent(EventDispatcher* dispatcher, const TextInputEvent& event);

      UIRendererPtr mRenderer;
      char* mName;

      float mMouseX;
      float mMouseY;
      float mDisplaySizeX;
      float mDisplaySizeY;
  };

  typedef std::unique_ptr<UIContext> UIContextPtr;

  /**
   * Abstract UI implementation
   */
  class GSAGE_API UIManager : public EventSubscriber<UIManager>
  {
    public:
      UIManager();
      virtual ~UIManager();

      inline bool initialized() const { return mInitialized; }

      /**
       * Updates UI state
       */
      virtual void newFrame(float delta);

      // interface functions ------------------------------------------

      /**
       * Does initial setup of the UIManager
       *
       * @param facade Gsage facade instance
       * @param L Initialize UI manager with lua state
       */
      virtual void initialize(GsageFacade* facade, lua_State* L = 0);

      /**
       * Create renderer
       */
      virtual void createRenderer(RenderSystem* render);

      /**
       * Shutdown renderer
       * Clear contexts
       */
      virtual void shutdown();

      /**
       * Get lua state
       */
      virtual lua_State* getLuaState();

      /**
       * Update lua state
       *
       * @param L New lua state
       */
      virtual void setLuaState(lua_State* L);

      // abstract

      /**
       * Get type id
       */
      virtual const std::string& getType() = 0;

      virtual UIRenderer::Configuration getRendererConfig() const
      {
        UIRenderer::Configuration res;
        memset(&res, 0, sizeof(UIRenderer::Configuration));
        return res;
      }

      /**
       * Create concrete UIManager Context
       */
      virtual UIContextPtr createContextImpl() const {
        // TODO: make it abstract later on
        return nullptr;
      }

    protected:

      GsageFacade* mFacade;
      lua_State* mLuaState;
      bool mInitialized;

      UIRendererPtr mRenderer;
      typedef std::unordered_map<std::string, UIContextPtr> Contexts;
      Contexts mContexts;

    private:

      bool handleMouseEvent(EventDispatcher* dispatcher, const Event& event);

      bool handleKeyboardEvent(EventDispatcher* dispatcher, const Event& event);

      bool handleInputEvent(EventDispatcher* dispatcher, const Event& event);

      bool handleSystemChange(EventDispatcher* dispatcher, const Event& event);

      bool handleCtxEvent(EventDispatcher* dispatcher, const Event& event);

      bool render(EventDispatcher* dispatcher, const Event& event);
  };
}

#endif
