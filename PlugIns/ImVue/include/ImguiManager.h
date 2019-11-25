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

#include "UIManager.h"
#include "EventSubscriber.h"
#include "MouseEvent.h"
#include "KeyboardEvent.h"
#include "ImguiExportHeader.h"
#include "DataProxy.h"
#include "imvue.h"
#include "imvue_generated.h"

struct lua_State;

namespace Gsage {
  class LuaInterface;
  class GsageFacade;
  class ImguiManager;

  /**
   * Runs scope in the context
   * Reverts context back at the end of the lifespan
   */
  struct ContextScope {
    ContextScope(ImGuiContext* ctx)
      : active(ctx)
      , backup(nullptr)
    {
      if(!ctx) {
        return;
      }
      backup = ImGui::GetCurrentContext();
      ImGui::SetCurrentContext(ctx);
    }

    ~ContextScope()
    {
      if(!backup)
        return;
      ImGui::SetCurrentContext(backup);
    }

    ImGuiContext* active;
    ImGuiContext* backup;
  };

  class ImguiContext : public UIContext
  {
    public:
      ImguiContext(GsageFacade* facade);
      ~ImguiContext();

      void newFrame(float delta);

      void render();

      void setRenderer(UIRendererPtr renderer);
      /**
       * Add new ImVue document to the imgui renderer
       */
      bool addDocument(const std::string& name, const std::string& file);

      /**
       * Gets wrapped imgui context
       */
      inline ImGuiContext* getWrappedContext()
      {
        return mContext;
      }

    protected:

      bool processInputEvent(EventDispatcher* sender, const TextInputEvent& event);
      bool processMouseEvent(EventDispatcher* sender, const MouseEvent& event);
      bool processKeyEvent(EventDispatcher* sender, const KeyboardEvent& event);

      bool captureMouse() const;
      bool captureKey() const;

    private:
      void configure();

      ImGuiContext* mContext;
      UIGeomPtr mRenderable;
      TexturePtr mFontTex;
      GsageFacade* mFacade;
      ImFontAtlas* mFontAtlas;
      ImVector<ImFont*> mFonts;
      typedef std::vector<ImVector<ImWchar>> GlyphRanges;
      GlyphRanges mGlyphRanges;
      typedef std::map<std::string, ImVue::Document*> Documents;
      Documents mDocuments;

      bool mFrameStarted;
      ImGuiMouseCursor mCurrentCursor;
  };

  class ImguiManager : public UIManager
  {
    public:
      static constexpr const char* TYPE = "ImGuiV2";

      ImguiManager();
      virtual ~ImguiManager();

      /**
       * Update load state
       * @param L lua_State
       */
      void setLuaState(lua_State* L);

      const std::string& getType()
      {
        static std::string t = TYPE;
        return t;
      }

      UIRenderer::Configuration getRendererConfig() const;

      inline ImguiContext* getContext(const std::string& name) {
        if(mContexts.find(name) != mContexts.end()) {
          return static_cast<ImguiContext*>(mContexts[name].get());
        }

        return nullptr;
      }
    protected:

      inline UIContextPtr createContextImpl() const
      {
        return std::move(UIContextPtr(new ImguiContext(mFacade)));
      }
  };
}
#endif
