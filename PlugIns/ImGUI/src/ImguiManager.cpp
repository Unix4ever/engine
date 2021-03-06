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

#include "ImguiManager.h"

#ifdef OGRE_INTERFACE
#include "ogre/ImguiOgreWrapper.h"
#include "ogre/OgreView.h"
#include "ogre/Gizmo.h"
#endif

#include "sol.hpp"
#include <imgui.h>

#include "MouseEvent.h"
#include "Engine.h"
#include "EngineSystem.h"
#include "EngineEvent.h"
#include "ImGuiDockspaceState.h"

namespace Gsage {

  bool ImguiRenderer::addView(const std::string& name, sol::object view, bool docked)
  {
    sol::protected_function callback;
    sol::table t = view.as<sol::table>();
    sol::optional<sol::protected_function> call = t[sol::meta_function::call];
    RenderView render;

    if (call) {
      render = [t, call] () -> sol::protected_function_result { return call.value()(t); };
      docked = t.get_or("docked", docked);
    } else if(view.is<sol::protected_function>()) {
      callback = view.as<sol::protected_function>();
      render = [callback] () -> sol::protected_function_result { return callback(); };
    } else {
      LOG(ERROR) << "Failed to add lua object as lua view " << name << ": must be either callable or function";
      return false;
    }

    if(docked){
      mDockedViews[name] = render;
    } else {
      mViews[name] = render;
    }

    return true;
  }

  bool ImguiRenderer::removeView(const std::string& name, sol::object view)
  {
    bool docked = false;
    auto views = &mViews;
    if(view != sol::lua_nil) {
      sol::table t = view.as<sol::table>();
      if(t.get_or("docked", docked)) {
        views = &mDockedViews;
      }
    }

    if(views->count(name) == 0) {
      return false;
    }

    views->erase(name);
    return true;
  }

  void ImguiRenderer::renderViews()
  {
    ImGuiIO& io = ImGui::GetIO();
    if(mDockedViews.size() > 0) {
      ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(2.0f, 2.0f));
      ImGui::PushStyleColor(ImGuiCol_WindowBg, ImGui::GetColorU32(ImGuiCol_Border));
      ImGui::PushStyleColor(ImGuiCol_ChildWindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
      ImVec2 size = io.DisplaySize;

      ImGui::SetNextWindowSize(size);
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      ImGui::Begin(
          "###dockspace",
          NULL,
          ImGuiWindowFlags_NoTitleBar |
          ImGuiWindowFlags_NoResize |
          ImGuiWindowFlags_NoMove |
          ImGuiWindowFlags_NoScrollbar |
          ImGuiWindowFlags_NoScrollWithMouse |
          ImGuiWindowFlags_NoBringToFrontOnFocus |
          ImGuiWindowFlags_NoSavedSettings
      );
      ImGui::PopStyleVar();
      mDockspace.beginWorkspace(ImVec2(5, 35), size - ImVec2(10, 40));
      ImGui::PopStyleColor(2);
      for(auto pair : mDockedViews) {
        try {
          auto res = pair.second();
          if(!res.valid()) {
            sol::error e = res;
            LOG(ERROR) << "Failed to render view " << pair.first << ": " << e.what();
          }
        } catch(sol::error e) {
          LOG(ERROR) << "Exception in view " << pair.first << ": " << e.what();
        }
      }
      mDockspace.endWorkspace();
      ImGui::End();
    }

    for(auto pair : mViews) {
      try {
        auto res = pair.second();
        if(!res.valid()) {
          sol::error e = res;
          LOG(ERROR) << "Failed to render view " << pair.first << ": " << e.what();
        }
      } catch(sol::error e) {
        LOG(ERROR) << "Exception in view " << pair.first << ": " << e.what();
      }
    }
  }

  void ImguiRenderer::setMousePosition(const std::string& name, ImVec2 position)
  {
    mMousePositions[name] = position;
  }

  ImguiManager::ImguiManager()
    : mRenderer(0)
    , mIsSetUp(false)
  {
  }

  ImguiManager::~ImguiManager()
  {
    if(mRenderer)
      delete mRenderer;
  }

  void ImguiManager::initialize(Engine* engine, lua_State* luaState)
  {
    UIManager::initialize(engine, luaState);
    setUp();
    addEventListener(engine, SystemChangeEvent::SYSTEM_ADDED, &ImguiManager::handleSystemChange);
    addEventListener(engine, SystemChangeEvent::SYSTEM_REMOVED, &ImguiManager::handleSystemChange);
  }

  bool ImguiManager::handleSystemChange(EventDispatcher* sender, const Event& event)
  {
    const SystemChangeEvent& e = static_cast<const SystemChangeEvent&>(event);
    if(e.mSystemId != "render") {
      return true;
    }

    if(e.getType() == SystemChangeEvent::SYSTEM_ADDED)
    {
      setUp();
    }

    if(e.getType() == SystemChangeEvent::SYSTEM_REMOVED && mRenderer != 0)
    {
      mIsSetUp = false;
      delete mRenderer;
      LOG(INFO) << "Render system was removed, system wrapper was destroyed";
      mRenderer = 0;
    }
    return true;
  }

  void ImguiManager::setUp()
  {
    if(mIsSetUp)
      return;

    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowTitleAlign = ImVec2(0.5f,0.5f);

    style.Colors[ImGuiCol_Text]                  = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
    style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
    style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.28f, 0.28f, 0.28f, 1.00f);
    style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_Border]                = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    //style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.32f, 0.32f, 0.32f, 1.00f);
    style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    style.Colors[ImGuiCol_Button]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.43f, 0.43f, 0.43f, 0.90f);
    style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
    style.Colors[ImGuiCol_Header]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_Column]                = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
    style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.59f, 0.59f, 0.59f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.98f, 0.39f, 0.36f, 1.00f);
    style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
    style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
    style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
    style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
    style.WindowPadding            = ImVec2(15, 15);
    style.WindowRounding           = 5.0f;
    style.ChildRounding            = 5.0f;
    style.FramePadding             = ImVec2(5, 5);
    style.FrameRounding            = 4.0f;
    style.ItemSpacing              = ImVec2(12, 8);
    style.ItemInnerSpacing         = ImVec2(8, 6);
    style.IndentSpacing            = 25.0f;
    style.ScrollbarSize            = 15.0f;
    style.ScrollbarRounding        = 10.0f;
    style.GrabMinSize              = 20.0f;
    style.GrabRounding             = 3.0f;

    EngineSystem* render = mEngine->getSystem("render");
    if(render == 0) {
      return;
    }

    const std::string type = render->getSystemInfo().get("type", "unknown");
    bool initialized = false;
#ifdef OGRE_INTERFACE
    if(type == "ogre") {
      if(mRenderer) {
        delete mRenderer;
      }
      LOG(INFO) << "Initialize for render system ogre3d";
      mRenderer = new ImguiOgreWrapper(mEngine);
      initialized = true;
    }

    setLuaState(mLuaState);
#endif

    if(!initialized) {
      LOG(ERROR) << "Failed to initialize imgui ui manager with the render system of type \"" << type << "\"";
      return;
    }

    ImGuiIO& io = ImGui::GetIO();
    io.KeyMap[ImGuiKey_Tab] = KeyboardEvent::KC_TAB;
    io.KeyMap[ImGuiKey_LeftArrow] = KeyboardEvent::KC_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = KeyboardEvent::KC_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = KeyboardEvent::KC_UP;
    io.KeyMap[ImGuiKey_DownArrow] = KeyboardEvent::KC_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = KeyboardEvent::KC_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = KeyboardEvent::KC_PGDOWN;
    io.KeyMap[ImGuiKey_Home] = KeyboardEvent::KC_HOME;
    io.KeyMap[ImGuiKey_End] = KeyboardEvent::KC_END;
    io.KeyMap[ImGuiKey_Delete] = KeyboardEvent::KC_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = KeyboardEvent::KC_BACK;
    io.KeyMap[ImGuiKey_Enter] = KeyboardEvent::KC_RETURN;
    io.KeyMap[ImGuiKey_Escape] = KeyboardEvent::KC_ESCAPE;
    io.KeyMap[ImGuiKey_A] = KeyboardEvent::KC_A;
    io.KeyMap[ImGuiKey_C] = KeyboardEvent::KC_C;
    io.KeyMap[ImGuiKey_V] = KeyboardEvent::KC_V;
    io.KeyMap[ImGuiKey_X] = KeyboardEvent::KC_X;
    io.KeyMap[ImGuiKey_Y] = KeyboardEvent::KC_Y;
    io.KeyMap[ImGuiKey_Z] = KeyboardEvent::KC_Z;
    // mouse events
    addEventListener(mEngine, MouseEvent::MOUSE_DOWN, &ImguiManager::handleMouseEvent, -100);
    addEventListener(mEngine, MouseEvent::MOUSE_UP, &ImguiManager::handleMouseEvent, -100);
    addEventListener(mEngine, MouseEvent::MOUSE_MOVE, &ImguiManager::handleMouseEvent, -100);
    // keyboard events
    addEventListener(mEngine, KeyboardEvent::KEY_DOWN, &ImguiManager::handleKeyboardEvent, -100);
    addEventListener(mEngine, KeyboardEvent::KEY_UP, &ImguiManager::handleKeyboardEvent, -100);
    // input event
    addEventListener(mEngine, TextInputEvent::INPUT, &ImguiManager::handleInputEvent, -100);
    mIsSetUp = true;
  }

  lua_State* ImguiManager::getLuaState()
  {
    return mLuaState;
  }

  void ImguiManager::setLuaState(lua_State* L)
  {
    mLuaState = L;
    sol::state_view lua(L);
    lua["imgui"]["render"] = mRenderer;
    lua["imgui"]["dockspace"] = mRenderer->mDockspace;

    lua["imgui"]["BeginDock"] = [this](std::string label, int flags) -> bool {
      return mRenderer->mDockspace.begin(label.c_str(), NULL, flags);
    };

    lua["imgui"]["BeginDockOpen"] = [this](std::string label, bool opened, int flags) -> std::tuple<bool, bool> {
      bool active = mRenderer->mDockspace.begin(label.c_str(), &opened, flags);
      return std::make_tuple(active, opened);
    };

    lua["imgui"]["EndDock"] = [this]() {
      return mRenderer->mDockspace.end();
    };

    lua["imgui"]["GetDockState"] = [this] () -> sol::table {
      DataProxy p = dumpState(mRenderer->mDockspace.getState());
      sol::state_view lua(mLuaState);
      sol::table t = lua.create_table();
      p.dump(t);
      return t;
    };

    lua["imgui"]["SetDockState"] = [this] (sol::table t) {
      DataProxy dp = DataProxy::wrap(t);
      ImGuiDockspaceState state = loadState(dp);
      mRenderer->mDockspace.setState(state);
    };

#ifdef OGRE_INTERFACE
    lua["imgui"]["createOgreView"] = [this] () -> std::shared_ptr<OgreView>{
      OgreRenderSystem* render = mEngine->getSystem<OgreRenderSystem>();
      if(render == 0) {
        return std::shared_ptr<OgreView>(nullptr);
      }
      return std::shared_ptr<OgreView>(new OgreView(render));
    };
    lua.new_usertype<OgreView>("OgreView",
        "setTextureID", &OgreView::setTextureID,
        "render", &OgreView::render
    );

    lua["imgui"]["createGizmo"] = [this] () -> std::shared_ptr<Gizmo>{
      OgreRenderSystem* render = mEngine->getSystem<OgreRenderSystem>();
      if(render == 0) {
        return std::shared_ptr<Gizmo>(nullptr);
      }
      return std::shared_ptr<Gizmo>(new Gizmo(render));
    };
    lua.new_usertype<Gizmo>("Gizmo",
        "setTarget", &Gizmo::setTarget,
        "enable", &Gizmo::enable,
        "render", &Gizmo::render,
        "operation", sol::property(&Gizmo::getOperation, &Gizmo::setOperation),
        "mode", sol::property(&Gizmo::getMode, &Gizmo::setMode),
        "drawCoordinatesEditor", &Gizmo::drawCoordinatesEditor
    );
#endif
  }

  bool ImguiManager::handleMouseEvent(EventDispatcher* sender, const Event& event)
  {
    if(!mRenderer)
      return true;

    const MouseEvent& e = static_cast<const MouseEvent&>(event);
    mRenderer->setMousePosition(e.dispatcher, ImVec2(e.mouseX, e.mouseY));
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += e.relativeZ / 100;

    io.MouseDown[e.button] = e.getType() == MouseEvent::MOUSE_DOWN;
    return !doCapture() || e.getType() == MouseEvent::MOUSE_UP;
  }

  bool ImguiManager::handleKeyboardEvent(EventDispatcher* sender, const Event& event)
  {
    if(!mRenderer)
      return true;

    const KeyboardEvent& e = static_cast<const KeyboardEvent&>(event);

    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[e.key] = e.getType() == KeyboardEvent::KEY_DOWN;

    if(e.text > 0)
    {
        io.AddInputCharacter((unsigned short)e.text);
    }

    // Read keyboard modifiers inputs
    io.KeyCtrl = e.isModifierDown(KeyboardEvent::Ctrl);
    io.KeyShift = e.isModifierDown(KeyboardEvent::Shift);
    io.KeyAlt = e.isModifierDown(KeyboardEvent::Alt);
    io.KeySuper = e.isModifierDown(KeyboardEvent::Win);

    return !ImGui::IsAnyItemActive() || e.getType() == KeyboardEvent::KEY_UP;
  }

  bool ImguiManager::handleInputEvent(EventDispatcher* sender, const Event& event)
  {
    if(!mRenderer)
      return true;

    const TextInputEvent& e = static_cast<const TextInputEvent&>(event);

    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(e.getText());

    return true;
  }

  bool ImguiManager::doCapture()
  {
    return ImGui::GetIO().WantCaptureMouse;
  }
}
