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

#include "ImguiDetachableWindow.h"
#include "GsageFacade.h"
#include "UIManager.h"
#include "ImguiManager.h"

namespace Gsage {

  ImguiDetachableWindow::ImguiDetachableWindow()
    : mDetached(false)
    , mWindow(nullptr)
    , mFacade(nullptr)
    , mForeignContext(nullptr)
    , name(nullptr)
    , flags(0)
    , open(true)
  {
  }

  ImguiDetachableWindow::~ImguiDetachableWindow()
  {
    if(name)
      ImGui::MemFree(name);
  }

  void ImguiDetachableWindow::renderBody()
  {
    ImGuiContext* ctx = nullptr;
    if(mForeignContext) {
      ctx = mForeignContext->getWrappedContext();
      mForeignContext->newFrame(ImGui::GetIO().DeltaTime);
    }
    ContextScope s(ctx);
    if(mDetached && mWindow) {
      ImGui::SetNextWindowPos(ImVec2(0, 0));
      std::tie(mSize.x, mSize.y) = mWindow->getSize();
      ImGui::SetNextWindowSize(mSize);
    }

    if(ImGui::Begin(name, &open, (mDetached ? DetachedFlags | flags : flags) | ImGuiWindowFlags_NoTitleBar)) {
      ImVue::ContainerElement::renderBody();
      mSize = ImGui::GetWindowSize();
    }
    ImGui::End();
  }

  void ImguiDetachableWindow::setDetached(bool value)
  {
    if(value == mDetached) {
      return;
    }

    if(!mFacade) {
      mFacade = (GsageFacade*)mCtx->userdata;
    }

    mDetached = value;

    if(value) {
      if(!mWindow) {
        DataProxy params;
        // configure window to inject render system
        params.put("render", true);
        params.put("highDPI", true);
        if((flags & ImGuiWindowFlags_NoResize) == 0) {
          params.put("resizable", true);
        }
        mWindow = mFacade->getWindowManager()->createWindow(name, mSize.x, mSize.y, false, params);
      }

      UIManager* manager = mFacade->getUIManager(ImguiManager::TYPE);
      GSAGE_ASSERT(manager != 0, "tried to get imgui ui manager and got nil");
      UIContext* ctx = manager->getContext(name);
      GSAGE_ASSERT(ctx != 0, "failed to get foreign context");
      mForeignContext = static_cast<ImguiContext*>(ctx);
      mWindow->bringToFront();
      mWindow->setSize(mSize.x, mSize.y);
      mWindow->show();
    } else {
      mForeignContext = nullptr;
      //mFacade->getWindowManager()->destroyWindow(mWindow);
      mWindow->hide();
      //mWindow = nullptr;
    }
  }
}
