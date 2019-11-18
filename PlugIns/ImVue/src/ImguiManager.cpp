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

#include "sol.hpp"
#include <imgui.h>
#include <lua/script.h>

#include "systems/RenderSystem.h"
#include "MouseEvent.h"
#include "Engine.h"
#include "GsageFacade.h"
#include "EngineSystem.h"
#include "EngineEvent.h"
#include "ImGuiConverters.h"
#include "ImguiEvent.h"
#include "ImguiImage.h"
#include "ImguiViewport.h"
#include "FileLoader.h"

namespace Gsage {

  /**
   * Runs scope in the context
   * Reverts context back at the end of the lifespan
   */
  struct ContextScope {
    ContextScope(ImGuiContext* ctx)
      : active(ctx)
    {
      backup = ImGui::GetCurrentContext();
      ImGui::SetCurrentContext(ctx);
    }

    ~ContextScope()
    {
      ImGui::SetCurrentContext(backup);
    }

    ImGuiContext* active;
    ImGuiContext* backup;
  };


  ImguiContext::ImguiContext(GsageFacade* facade)
    : mFacade(facade)
    , mContext(0)
    , mFontAtlas(0)
    , mFontTex(nullptr)
    , mFrameStarted(false)
    , mCurrentCursor(ImGuiMouseCursor_Arrow)
  {
  }

  ImguiContext::~ImguiContext()
  {
    if(mContext) {
      ImGui::DestroyContext(mContext);
    }
  }

  void ImguiContext::newFrame(float delta)
  {
    if(!mContext || mFrameStarted) {
      return;
    }
    mFrameStarted = true;

    ContextScope cs(mContext);
    ImGuiIO& io = ImGui::GetIO();
    io.DeltaTime = std::max(
        delta,
        1e-4f); // see https://github.com/ocornut/imgui/commit/3c07ec6a6126fb6b98523a9685d1f0f78ca3c40c

    // Setup display size (every frame to accommodate for window resizing)
    io.DisplaySize = ImVec2(mDisplaySizeX, mDisplaySizeX);
    io.MousePos = ImVec2(mMouseX, mMouseY);
    // Start the frame
    ImGui::NewFrame();

    // render ImVue documents
    for(auto& pair : mDocuments) {
      try {
        pair.second->render();
      } catch (const ImVue::ElementError& err) {
        LOG(ERROR) << "Failed to render document " << pair.first << " " << err.what();
      } catch (const ImVue::ScriptError& err) {
        LOG(ERROR) << "Failed to render document " << pair.first << " " << err.what();
      } catch (...) {
        LOG(ERROR) << "Failed to render document " << pair.first;
      }
    }
  }

  void ImguiContext::render()
  {
    if(!mRenderer || !mFrameStarted) {
      return;
    }

    if(!mRenderable) {
      mRenderable = mRenderer->createUIGeom();
    }

    ContextScope cs(mContext);

    ImGui::EndFrame();
    ImGui::Render();
    mRenderer->begin(mName);
    ImDrawData *drawData = ImGui::GetDrawData();
    int numberDraws = 0;
    for(int n = 0; n < drawData->CmdListsCount; n++) {
      const ImDrawList* drawList = drawData->CmdLists[n];
      const ImDrawVert* vtxBuf = drawList->VtxBuffer.Data;
      const ImDrawIdx* idxBuf = drawList->IdxBuffer.Data;
      int current = 0;
      unsigned int startIdx = 0;

      for (int i = 0; i < drawList->CmdBuffer.Size; i++)
      {
        const ImDrawCmd *drawCmd = &drawList->CmdBuffer[current];

        mRenderable->updateVertexData(
            vtxBuf,
            &idxBuf[startIdx],
            drawList->VtxBuffer.Size,
            drawCmd->ElemCount,
            sizeof(ImDrawVert),
            sizeof(ImDrawIdx)
        );
        // Set scissoring
        mRenderable->scLeft = static_cast<int>(drawCmd->ClipRect.x); // Obtain bounds
        mRenderable->scTop = static_cast<int>(drawCmd->ClipRect.y);
        mRenderable->scRight = static_cast<int>(drawCmd->ClipRect.z);
        mRenderable->scBottom = static_cast<int>(drawCmd->ClipRect.w);
        if (drawCmd->TextureId) {
          auto handle = (TextureHandle*)drawCmd->TextureId;
          mRenderable->texture = handle->texture;
        } else {
          mRenderable->texture = mFontTex;
        }
        mRenderer->renderUIGeom(mRenderable);
        current++;
        startIdx += drawCmd->ElemCount;
      }
    }
    mRenderer->end();
    if(mCurrentCursor != ImGui::GetMouseCursor()) {
      mCurrentCursor = ImGui::GetMouseCursor();

      WindowPtr window = mRenderer->getViewport(mName)->getWindow();
      if(window) {
        window->setCursor((Window::Cursor)mCurrentCursor);
      }
    }
    mFrameStarted = false;
  }

  void ImguiContext::setRenderer(UIRendererPtr renderer)
  {
    mRenderable = nullptr;
    mFontTex = nullptr;
    UIContext::setRenderer(renderer);
    if(renderer) {
      configure();
    }
  }

  void ImguiContext::configure()
  {
    WindowPtr window = mRenderer->getViewport(mName)->getWindow();
    float scale = 1.0f;
    if(window) {
      scale = std::max(1.0f, window->getScaleFactor() * .9f);
    }

    if(!mContext) {
      LOG(INFO) << "Creating ImGui context " << mName;
      mContext = ImGui::CreateContext(mFontAtlas);

      DataProxy theme = DataProxy::create(DataWrapper::JSON_OBJECT);
      if(!mFacade->getEngine()->settings().read(std::string("imgui.theme.") + mName, theme)) {
        mFacade->getEngine()->settings().read("imgui.theme", theme);
      }

      ImGuiStyle& style = ImGui::GetStyle();

      DataProxy colors = theme.get("colors", DataProxy::create(DataWrapper::JSON_OBJECT));

      style.Colors[ImGuiCol_Text]                  = colors.get("text", ImVec4(0.73f, 0.73f, 0.73f, 1.00f));
      style.Colors[ImGuiCol_TextDisabled]          = colors.get("textDisabled", ImVec4(0.50f, 0.50f, 0.50f, 1.00f));
      style.Colors[ImGuiCol_WindowBg]              = colors.get("windowBg", ImVec4(0.26f, 0.26f, 0.26f, 0.95f));
      style.Colors[ImGuiCol_ChildWindowBg]         = colors.get("childWindowBg", ImVec4(0.28f, 0.28f, 0.28f, 1.00f));
      style.Colors[ImGuiCol_PopupBg]               = colors.get("popupBg", ImVec4(0.26f, 0.26f, 0.26f, 1.00f));
      style.Colors[ImGuiCol_Border]                = colors.get("border", ImVec4(0.11f, 0.11f, 0.11f, 1.00f));
      style.Colors[ImGuiCol_BorderShadow]          = colors.get("borderShadow", ImVec4(0.11f, 0.11f, 0.11f, 1.00f));
      style.Colors[ImGuiCol_FrameBg]               = colors.get("frameBg", ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
      style.Colors[ImGuiCol_FrameBgHovered]        = colors.get("frameBgHovered", ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
      style.Colors[ImGuiCol_FrameBgActive]         = colors.get("frameBgActive", ImVec4(0.16f, 0.16f, 0.16f, 1.00f));
      style.Colors[ImGuiCol_TitleBg]               = colors.get("titleBg", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_TitleBgCollapsed]      = colors.get("titleBgCollapsed", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_TitleBgActive]         = colors.get("titleBgActive", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_MenuBarBg]             = colors.get("menuBarBg", ImVec4(0.26f, 0.26f, 0.26f, 1.00f));
      style.Colors[ImGuiCol_ScrollbarBg]           = colors.get("scrollbarBg", ImVec4(0.21f, 0.21f, 0.21f, 1.00f));
      style.Colors[ImGuiCol_ScrollbarGrab]         = colors.get("scrollbarGrab", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_ScrollbarGrabHovered]  = colors.get("scrollbarGrabHovered", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_ScrollbarGrabActive]   = colors.get("scrollbarGrabActive", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_CheckMark]             = colors.get("checkMark", ImVec4(0.78f, 0.78f, 0.78f, 1.00f));
      style.Colors[ImGuiCol_SliderGrab]            = colors.get("sliderGrab", ImVec4(0.74f, 0.74f, 0.74f, 1.00f));
      style.Colors[ImGuiCol_SliderGrabActive]      = colors.get("sliderGrabActive", ImVec4(0.74f, 0.74f, 0.74f, 1.00f));
      style.Colors[ImGuiCol_Button]                = colors.get("button", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_ButtonHovered]         = colors.get("buttonHovered", ImVec4(0.43f, 0.43f, 0.43f, 0.90f));
      style.Colors[ImGuiCol_ButtonActive]          = colors.get("buttonActive", ImVec4(0.11f, 0.11f, 0.11f, 1.00f));
      style.Colors[ImGuiCol_Header]                = colors.get("header", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_HeaderHovered]         = colors.get("headerHovered", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_HeaderActive]          = colors.get("headerActive", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_ResizeGrip]            = colors.get("resizeGrip", ImVec4(0.36f, 0.36f, 0.36f, 1.00f));
      style.Colors[ImGuiCol_ResizeGripHovered]     = colors.get("resizeGripHovered", ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
      style.Colors[ImGuiCol_ResizeGripActive]      = colors.get("resizeGripActive", ImVec4(0.26f, 0.59f, 0.98f, 1.00f));
      style.Colors[ImGuiCol_PlotLines]             = colors.get("plotLines", ImVec4(0.39f, 0.39f, 0.39f, 1.00f));
      style.Colors[ImGuiCol_PlotLinesHovered]      = colors.get("plotLinesHovered", ImVec4(1.00f, 0.43f, 0.35f, 1.00f));
      style.Colors[ImGuiCol_PlotHistogram]         = colors.get("plotHistogram", ImVec4(0.90f, 0.70f, 0.00f, 1.00f));
      style.Colors[ImGuiCol_PlotHistogramHovered]  = colors.get("plotHistogramHovered", ImVec4(1.00f, 0.60f, 0.00f, 1.00f));
      style.Colors[ImGuiCol_TextSelectedBg]        = colors.get("textSelectedBg", ImVec4(0.32f, 0.52f, 0.65f, 1.00f));
      style.Colors[ImGuiCol_ModalWindowDarkening]  = colors.get("modalWindowDarkening", ImVec4(0.20f, 0.20f, 0.20f, 0.50f));
      style.Colors[ImGuiCol_NavHighlight]          = colors.get("navHighlight", ImVec4(1.00f, 1.00f, 1.00f, 1.00f));

      style.WindowTitleAlign      = theme.get("windowTitleAlign", ImVec2(0.5f,0.5f));
      style.WindowPadding         = theme.get("windowPadding", ImVec2(15, 15));
      style.WindowRounding        = theme.get("windowRounding", 5.0f);
      style.ChildRounding         = theme.get("childRounding", 5.0f);
      style.FramePadding          = theme.get("framePadding", ImVec2(5.0f, 5.0f));
      style.FrameBorderSize       = theme.get("frameBorderSize", 0.0f);
      style.ButtonTextAlign       = theme.get("buttonTextAlign", ImVec2(0.5f,0.3f));
      style.FrameRounding         = theme.get("frameRounding", 4.0f);
      style.ItemSpacing           = theme.get("itemSpacing", ImVec2(12.0f, 8.0f));
      style.ItemInnerSpacing      = theme.get("itemInnerSpacing", ImVec2(8.0f, 6.0f));
      style.IndentSpacing         = theme.get("indentSpacing", 25.0f);
      style.ScrollbarSize         = theme.get("scrollbarSize", 15.0f);
      style.ScrollbarRounding     = theme.get("scrollbarRounding", 10.0f);
      style.GrabMinSize           = theme.get("grabMinSize", 20.0f);
      style.GrabRounding          = theme.get("grabRounding", 3.0f);
      style.CurveTessellationTol  = theme.get("curveTessellationTol", 1.25f);

      style.ScaleAllSizes(scale);
    }

    ContextScope cs(mContext);
    ImGuiIO& io = ImGui::GetIO();
    io.DisplayFramebufferScale = ImVec2(scale, scale);
    if(!mFontAtlas) {
      auto additionalFonts = mFacade->getEngine()->settings().get<DataProxy>("imgui.fonts");
      // Build texture atlas
      if(additionalFonts.second) {
        int i = 0;
        mGlyphRanges.clear();
        mGlyphRanges.reserve(additionalFonts.first.size());
        for(auto pair : additionalFonts.first) {
          mGlyphRanges.emplace_back();
          auto file = pair.second.get<std::string>("file");
          if(!file.second) {
            LOG(ERROR) << "Malformed font syntax, \'file\' field is mandatory";
            continue;
          }

          auto size = pair.second.get<float>("size");
          if(!size.second) {
            LOG(ERROR) << "Malformed font syntax, \'size\' field is mandatory";
            continue;
          }

          ImFontConfig config;
          config.MergeMode = pair.second.get("merge", false);
          pair.second.read("oversampleH", config.OversampleH);
          pair.second.read("oversampleV", config.OversampleV);

          ImFontAtlas::GlyphRangesBuilder builder;
          auto glyphRanges = pair.second.get<DataProxy>("glyphRanges");
          if(glyphRanges.second) {
            for(auto range : glyphRanges.first) {
              for(int j = range.second[0].as<int>(); j < range.second[1].as<int>(); j++) {
                builder.AddChar((ImWchar)j);
              }
            }
          }

          if(!pair.second.get("noDefaultRanges", false)) {
            builder.AddRanges(io.Fonts->GetGlyphRangesJapanese());
            builder.AddRanges(io.Fonts->GetGlyphRangesDefault());
            builder.AddRanges(io.Fonts->GetGlyphRangesCyrillic());
          }

          auto glyphOffset = pair.second.get<ImVec2>("glyphOffset");
          if(glyphOffset.second) {
            config.GlyphOffset = glyphOffset.first * scale;
          }
          builder.BuildRanges(&mGlyphRanges[i]);
          config.PixelSnapH = pair.second.get("pixelSnapH", false);

          std::string path = FileLoader::getSingletonPtr()->searchFile(file.first);
          if(path.empty()) {
            LOG(WARNING) << "Skipped loading font " << path << ": file not found";
            continue;
          }

          ImFont* pFont = io.Fonts->AddFontFromFileTTF(path.c_str(), (int)((float)size.first * scale), &config, mGlyphRanges[i].Data);
          mFonts.push_back(pFont);
          i++;
        }
      }

      mFontAtlas = io.Fonts;
    }

    if(!mFontTex) {
      unsigned char* pixels;
      int width, height;
      mFontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height);
      if(mRenderer)
        mFontTex = mRenderer->createTexture("imguiFontTexture", pixels, width, height, PF_R8G8B8A8);
    }

    io.MouseDrawCursor = mFacade->getEngine()->settings().get("imgui.drawCursor", false);
    io.DisplaySize = ImVec2(mDisplaySizeX, mDisplaySizeY);
    ImGui::NewFrame();
    ImGui::EndFrame();

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
    io.KeyMap[ImGuiKey_Space] = KeyboardEvent::KC_SPACE;

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendPlatformName = "imgui_impl_gsage";
    mFacade->getEngine()->fireEvent(ImguiEvent(ImguiEvent::CONTEXT_CREATED, mName));
  }

  bool ImguiContext::addDocument(const std::string& name, const std::string& file)
  {
    ImVue::ElementFactory* factory = ImVue::createElementFactory();
    factory->element<ImguiViewport>("viewport")
      .attribute("bg-colour", &ImguiViewport::bgColour)
      .attribute("texture", &ImguiViewport::textureID);

    ContextScope cs(mContext);
    ImVue::Context* ctx = ImVue::createContext(
        factory,
        new ImVue::LuaScriptState(mFacade->getLuaState()),
        // TODO: custom filesystem and texture manager
        0,
        0,
        mFacade
    );

    ImVue::Document* doc = new ImVue::Document(ctx);
    std::string path = FileLoader::getSingletonPtr()->searchFile(file);
    if(path.empty()) {
      LOG(ERROR) << "Document file not found " << file;
      return false;
    }
    char* data = ctx->fs->load(&path[0]);
    if(!data) {
      return false;
    }

    doc->parse(data);
    ImGui::MemFree(data);
    mDocuments[name] = doc;
    return true;
  }

  bool ImguiContext::processMouseEvent(EventDispatcher* sender, const MouseEvent& e)
  {
    ContextScope cs(mContext);
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheel += e.relativeZ / 100;
    io.MouseDown[e.button] = e.getType() == MouseEvent::MOUSE_DOWN;

    return UIContext::processMouseEvent(sender, e);
  }

  bool ImguiContext::processKeyEvent(EventDispatcher* sender, const KeyboardEvent& e)
  {
    ContextScope cs(mContext);
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[e.key] = e.getType() == KeyboardEvent::KEY_DOWN;

    // Read keyboard modifiers inputs
    io.KeyCtrl = e.isModifierDown(KeyboardEvent::Ctrl);
    io.KeyShift = e.isModifierDown(KeyboardEvent::Shift);
    io.KeyAlt = e.isModifierDown(KeyboardEvent::Alt);
    io.KeySuper = e.isModifierDown(KeyboardEvent::Win);

    return UIContext::processKeyEvent(sender, e);
  }

  bool ImguiContext::processInputEvent(EventDispatcher* sender, const TextInputEvent& e)
  {
    ContextScope cs(mContext);
    ImGuiIO& io = ImGui::GetIO();
    io.AddInputCharactersUTF8(e.getText());
    return UIContext::processInputEvent(sender, e);
  }

  bool ImguiContext::captureMouse() const
  {
    return ImGui::GetIO().WantCaptureMouse;
  }

  bool ImguiContext::captureKey() const
  {
    return ImGui::IsAnyItemActive();
  }

  ImguiManager::ImguiManager()
  {
  }

  ImguiManager::~ImguiManager()
  {
  }

  UIRenderer::Configuration ImguiManager::getRendererConfig() const
  {
    // set up renderer configuration
    UIRenderer::Configuration config = UIRenderer::createConfiguration();
    config.valid = true;
    config.blendType = SceneBlendType::SBT_TRANSPARENT_ALPHA;
    config.blendOperation = SceneBlendOperation::SBO_ADD;
    config.blendOperationAlpha = SceneBlendOperation::SBO_ADD;
    config.srcBlendFactor = SceneBlendFactor::SBF_SOURCE_ALPHA;
    config.dstBlendFactor = SceneBlendFactor::SBF_ONE_MINUS_SOURCE_ALPHA;
    config.srcBlendFactorAlpha = SceneBlendFactor::SBF_ONE_MINUS_SOURCE_ALPHA;
    config.dstBlendFactorAlpha = SceneBlendFactor::SBF_ZERO;
    config.cullMode = CullingMode::CULL_NONE;
    config.texFiltering = TextureFilterOptions::TFO_NONE;
    return config;
  }


  void ImguiManager::setLuaState(lua_State* L)
  {
    mLuaState = L;
    ImVue::registerBindings(L);
  }
}
