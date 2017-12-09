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

#include "ImguiPlugin.h"
#include "GsageFacade.h"
#include "ImguiLuaInterface.h"
#include <imgui.h>

#include "ImGuizmo.h"

#include "sol.hpp"

namespace Gsage {

  const std::string PLUGIN_NAME = "Imgui";

  ImguiPlugin::ImguiPlugin()
    : mUIManagerHandle(-1)
  {
  }

  const std::string& ImguiPlugin::getName() const
  {
    return PLUGIN_NAME;
  }

  ImguiPlugin::~ImguiPlugin()
  {
  }

  bool ImguiPlugin::installImpl()
  {
    mUIManagerHandle = mFacade->addUIManager(&mUIManager);
    return true;
  }

  void ImguiPlugin::uninstallImpl()
  {
    sol::state_view& lua = *mLuaInterface->getSolState();
    lua["imgui"] = sol::lua_nil;
    lua["ImguiRenderer"] = sol::lua_nil;

    mFacade->removeUIManager(mUIManagerHandle);
  }

  void ImguiPlugin::setupLuaBindings()
  {
    sol::state_view& lua = *mLuaInterface->getSolState();

    lua.new_usertype<ImguiRenderer>("ImguiRenderer",
        "new", sol::no_constructor,
        "addView", &ImguiRenderer::addView,
        "removeView", &ImguiRenderer::removeView
    );

    ImguiLuaInterface::addLuaBindings(lua);

    sol::table gizmo = lua.create_table();

    gizmo["IsOver"] = [] () -> bool { return ImGuizmo::IsOver(); };
    gizmo["IsUsing"] = [] () -> bool { return ImGuizmo::IsUsing(); };
    gizmo["ROTATE"] = ImGuizmo::OPERATION::ROTATE;
    gizmo["TRANSLATE"] = ImGuizmo::OPERATION::TRANSLATE;
    gizmo["SCALE"] = ImGuizmo::OPERATION::SCALE;

    gizmo["WORLD"] = ImGuizmo::MODE::WORLD;
    gizmo["LOCAL"] = ImGuizmo::MODE::LOCAL;

    lua["imgui"]["gizmo"] = gizmo;
    lua["imgui"]["BeginDock"] = [](std::string label, int flags) -> bool {
      return igBeginDock(label.c_str(), NULL, flags);
    };
    lua["imgui"]["BeginDock"] = [](std::string label, bool opened, int flags) -> bool {
      igBeginDock(label.c_str(), &opened, flags);
      return opened;
    };
    lua["imgui"]["EndDock"] = igEndDock;
    lua["imgui"]["SetDockActive"] = igSetDockActive;
    lua["imgui"]["ShutdownDock"] = igShutdownDock;
    lua["imgui"]["GetDockState"] = getDockState;
    lua["imgui"]["SetDockState"] = setDockState;
  }
}

Gsage::ImguiPlugin* imguiPlugin = NULL;

extern "C" bool PluginExport dllStartPlugin(Gsage::GsageFacade* facade)
{
  if(imguiPlugin != NULL)
  {
    return false;
  }
  imguiPlugin = new Gsage::ImguiPlugin();
  return facade->installPlugin(imguiPlugin);
}

extern "C" bool PluginExport dllStopPlugin(Gsage::GsageFacade* facade)
{
  if(imguiPlugin == NULL)
    return true;

  bool res = facade->uninstallPlugin(imguiPlugin);
  if(!res)
    return false;
  delete imguiPlugin;
  imguiPlugin = NULL;
  return true;
}
