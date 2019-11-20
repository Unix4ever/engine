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

#include "UIManager.h"
#include "UIContextEvent.h"

namespace Gsage {


  bool UIContext::captureMouse() const
  {
    return false;
  }

  bool UIContext::captureKey() const
  {
    return false;
  }

  bool UIContext::captureInput() const
  {
    return false;
  }

  bool UIContext::processKeyEvent(EventDispatcher* dispatcher, const KeyboardEvent& event)
  {
    return !captureKey() || event.getType() == KeyboardEvent::KEY_UP;
  }

  bool UIContext::processMouseEvent(EventDispatcher* dispatcher, const MouseEvent& event)
  {
    updateMousePos(event.mouseX, event.mouseY);
    return !captureMouse() || event.getType() == MouseEvent::MOUSE_UP;
  }

  bool UIContext::processInputEvent(EventDispatcher* dispatcher, const TextInputEvent& event)
  {
    return !captureInput();
  }

  UIManager::UIManager()
    : mLuaState(nullptr)
    , mFacade(nullptr)
    , mInitialized(false)
    , mRenderer(nullptr)
  {
  }

  UIManager::~UIManager()
  {
  }

  void UIManager::newFrame(float delta)
  {
    for(auto& pair : mContexts) {
      pair.second->newFrame(delta);
    }
  }

  bool UIManager::render(EventDispatcher* dispatcher, const Event& event) {
    const UIContextEvent& e = static_cast<const UIContextEvent&>(event);
    if(mContexts.count(e.ctxName) != 0) {
      mContexts[e.ctxName]->render();
    }

    return true;
  }

  void UIManager::initialize(GsageFacade* facade, lua_State* L) {
    mFacade = facade;
    mLuaState = L;
    mInitialized = true;

    Engine* engine = facade->getEngine();
    addEventListener(engine, SystemChangeEvent::SYSTEM_STOPPING, &UIManager::handleSystemChange);
    addEventListener(engine, SystemChangeEvent::SYSTEM_STARTED, &UIManager::handleSystemChange);
    // mouse events
    addEventListener(engine, MouseEvent::MOUSE_DOWN, &UIManager::handleMouseEvent, -100);
    addEventListener(engine, MouseEvent::MOUSE_UP, &UIManager::handleMouseEvent, -100);
    addEventListener(engine, MouseEvent::MOUSE_MOVE, &UIManager::handleMouseEvent, -100);
    // keyboard events
    addEventListener(engine, KeyboardEvent::KEY_DOWN, &UIManager::handleKeyboardEvent, -100);
    addEventListener(engine, KeyboardEvent::KEY_UP, &UIManager::handleKeyboardEvent, -100);
    // input event
    addEventListener(engine, TextInputEvent::INPUT, &UIManager::handleInputEvent, -100);

    // context events
    addEventListener(engine, UIContextEvent::CREATE, &UIManager::handleCtxEvent);
    addEventListener(engine, UIContextEvent::RESIZE, &UIManager::handleCtxEvent);
    addEventListener(engine, UIContextEvent::DESTROY, &UIManager::handleCtxEvent);
    addEventListener(engine, UIContextEvent::RENDER, &UIManager::render);
  }

  void UIManager::createRenderer(RenderSystem* render)
  {
    mRenderer = render->createUIRenderer();
    UIRenderer::Configuration config = getRendererConfig();
    GSAGE_ASSERT(config.valid, "UI renderer config wasn't properly set up");
    if(mRenderer) {
      mRenderer->initialize(
          config,
          render,
          getType()
      );

      for(auto& pair : mContexts) {
        pair.second->setRenderer(mRenderer);
      }
    }
  }

  void UIManager::shutdown()
  {
    mRenderer = nullptr;
    for(auto& pair : mContexts) {
      pair.second->setRenderer(nullptr);
    }
  }

  lua_State* UIManager::getLuaState() {
    return mLuaState;
  }

  void UIManager::setLuaState(lua_State* L) {
    mLuaState = L;
  };

  bool UIManager::handleMouseEvent(EventDispatcher* dispatcher, const Event& event)
  {
    const MouseEvent& e = static_cast<const MouseEvent&>(event);
    if(mContexts.count(e.dispatcher) == 0) {
      return true;
    }

    mContexts[e.dispatcher]->processMouseEvent(dispatcher, e);
    return true;
  }

  bool UIManager::handleKeyboardEvent(EventDispatcher* dispatcher, const Event& event)
  {
    for(auto& pair : mContexts) {
      if(!pair.second->processKeyEvent(dispatcher, static_cast<const KeyboardEvent&>(event))) {
        return false;
      }
    }

    return true;
  }

  bool UIManager::handleInputEvent(EventDispatcher* dispatcher, const Event& event)
  {
    for(auto& pair : mContexts) {
      if(!pair.second->processInputEvent(dispatcher, static_cast<const TextInputEvent&>(event))) {
        return false;
      }
    }

    return true;
  }

  bool UIManager::handleSystemChange(EventDispatcher* dispatcher, const Event& event)
  {
    const SystemChangeEvent& e = static_cast<const SystemChangeEvent&>(event);

    if(e.mSystemId != "render") {
      return true;
    }

    if(e.getType() == SystemChangeEvent::SYSTEM_STARTED)
    {
      RenderSystem* render = dynamic_cast<RenderSystem*>(mFacade->getEngine()->getSystem("render"));
      createRenderer(render);
      if(mRenderer) {
        LOG(INFO) << "[" << getType() << "] Render system started, created UI renderer";
      }
    }

    if(e.getType() == SystemChangeEvent::SYSTEM_STOPPING)
    {
      shutdown();
      LOG(INFO) << "[" << getType() << "] Render system was removed, destroyed UI renderer";
    }
    return true;
  }

  bool UIManager::handleCtxEvent(EventDispatcher* dispatcher, const Event& event)
  {
    const UIContextEvent& e = static_cast<const UIContextEvent&>(event);

    if(event.getType() == UIContextEvent::CREATE) {
      if(mContexts.count(e.ctxName) == 0) {
        mContexts[e.ctxName] = createContextImpl();
        mContexts[e.ctxName]->mName = strdup(e.ctxName.c_str());
      }

      mContexts[e.ctxName]->resize(e.width, e.height);
      mContexts[e.ctxName]->setRenderer(mRenderer);
      LOG(INFO) << "[" << getType() << "] Configure context " << e.ctxName;
    } else if(event.getType() == UIContextEvent::RESIZE) {
      if(mContexts.find(e.ctxName) != mContexts.end()) {
        mContexts[e.ctxName]->resize(e.width, e.height);
        LOG(INFO) << "[" << getType() << "] Resize context " << e.ctxName;
      }
    } else if(event.getType() == UIContextEvent::DESTROY) {
      if(mContexts.count(e.ctxName) == 0) {
        return true;
      }

      mContexts.erase(e.ctxName);
      LOG(INFO) << "[" << getType() << "] Destroy context " << e.ctxName;
    }
    return true;
  }
}
