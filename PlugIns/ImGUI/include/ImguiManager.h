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

#ifndef _ImguiManager_H_
#define _ImguiManager_H_

#include <Rocket/Core/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Core/RenderInterface.h>
#include <Rocket/Core/SystemInterface.h>

#include "UIManager.h"
#include "EventSubscriber.h"
#include "KeyboardEvent.h"

#include "sol.hpp"

struct lua_State;

namespace Gsage {
  class LuaInterface;
  class RenderEvent;
  class Engine;

  class ImguiRenderer {
    public:
      ImguiRenderer() {}
      virtual ~ImguiRenderer() {}

      /**
       * Add new lua view to the imgui renderer.
       *
       * @param name view name
       * @param view lua protected function that will render everything.
       */
      void addView(const std::string& name, sol::object view);

      /**
       * Remove named view
       *
       * @param name view name
       * @returns true if the view was removed
       */
      bool removeView(const std::string& name);
    protected:
      virtual void renderViews();

      typedef std::function<sol::protected_function()> RenderView;

      typedef std::map<std::string, RenderView> Views;

      Views mViews;
  };

  class ImguiManager : public UIManager, public EventSubscriber<ImguiManager>
  {
    public:
      ImguiManager();
      virtual ~ImguiManager();
      /**
       * Initialize ui manager
       *
       * @param called by gsage facade on setup
       * @param L init with lua state
       */
      virtual void initialize(Engine* engine, lua_State* L = 0);
      /**
       * Gets Rocket lua state
       */
      lua_State* getLuaState();

      /**
       * Update load state
       * @param L lua_State
       */
      void setLuaState(lua_State* L);

      /**
       * Configures rendering
       */
      void setUp();

      /**
       * SystemChangeEvent::SYSTEM_ADDED and SystemChangeEvent::SYSTEM_REMOVED handler
       */
      bool handleSystemChange(EventDispatcher* sender, const Event& event);
    private:
      /**
       * Handle mouse event from engine
       *
       * @param event Event
       */
      bool handleMouseEvent(EventDispatcher* sender, const Event& event);
      /**
       * Handle keyboard event from engine
       */
      bool handleKeyboardEvent(EventDispatcher* sender, const Event& event);
      /**
       * Check if mouse event can be captured by any rocket element
       */
      bool doCapture();

      ImguiRenderer* mRenderer;

      bool mIsSetUp;

  };
}
#endif
