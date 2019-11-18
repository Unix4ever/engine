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

#include "ImguiViewport.h"
#include "GsageFacade.h"
#include "imvue_errors.h"

namespace Gsage {

  ImguiViewport::ImguiViewport()
    : bgColour(0)
    , textureID(nullptr)
    , mDirty(false)
    , mRender(nullptr)
    , mTextureID(nullptr)
    , mTexture(nullptr)
    , mUV1(1.0f, 1.0f)
  {
  }

  ImguiViewport::~ImguiViewport()
  {
    if(textureID)
      ImGui::MemFree(textureID);

    if(mTexture) {
      removeEventListener(mTexture.get(), Texture::RECREATE, &ImguiViewport::onTextureEvent);
      removeEventListener(mTexture.get(), Texture::DESTROY, &ImguiViewport::onTextureEvent);
      removeEventListener(mTexture.get(), Texture::UV_UPDATE, &ImguiViewport::onTextureEvent);
    }
  }

  void ImguiViewport::renderBody()
  {
    if(!mRender) {
      GsageFacade* facade = ((GsageFacade*)mCtx->userdata);
      mRender = dynamic_cast<RenderSystem*>(facade->getEngine()->getSystem("render"));
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->PathClear();
    ImVec2 size = ImGui::GetContentRegionAvail();
    drawList->AddRectFilled(ImVec2(pos.x, pos.y), pos + size, bgColour);

    if(!textureID || !mRender) {
      return;
    }

    if(!mTexture || !textureID || strcmp(mTextureID, textureID) != 0 || mDirty) {
      mTextureID = textureID;
      if(textureID) {
        mTexture = mRender->getTexture(mTextureID);
      } else {
        mTexture = NULL;
      }

      mTextureHandle.texture = mTexture;
    }

    if(!mTexture || !mTexture->hasData() || size.x <= 0 || size.y <= 0) {
      return;
    }

    RenderViewport* viewport = mRender->getViewport(mTextureID);
    viewport->setDimensions(size.x, size.y);
    viewport->setPosition(pos.x, pos.y);
    Gsage::Vector2 texSize = mTexture->getSrcSize();
    mSize = ImVec2(texSize.X, texSize.Y);
    ImGui::Image(&mTextureHandle, size, mUV0, mUV1);
  }

  void ImguiViewport::setTexture(TexturePtr texture)
  {
    if(mTexture) {
      removeEventListener(mTexture.get(), Texture::RECREATE, &ImguiViewport::onTextureEvent);
      removeEventListener(mTexture.get(), Texture::DESTROY, &ImguiViewport::onTextureEvent);
      removeEventListener(mTexture.get(), Texture::UV_UPDATE, &ImguiViewport::onTextureEvent);
    }
    mTexture = texture;
    addEventListener(texture.get(), Texture::RECREATE, &ImguiViewport::onTextureEvent);
    addEventListener(texture.get(), Texture::DESTROY, &ImguiViewport::onTextureEvent);
    addEventListener(texture.get(), Texture::UV_UPDATE, &ImguiViewport::onTextureEvent);
  }

  bool ImguiViewport::onTextureEvent(EventDispatcher* sender, const Event& event)
  {
    if(event.getType() == Texture::RECREATE) {
      mTexture = NULL;
      mDirty = true;
    } else if(event.getType() == Texture::UV_UPDATE) {
      Gsage::Vector2 tl;
      Gsage::Vector2 bl;
      Gsage::Vector2 tr;
      Gsage::Vector2 br;
      std::tie(tl, bl, tr, br) = mTexture->getUVs();
      mUV0 = ImVec2(tl.X, tl.Y);
      mUV1 = ImVec2(br.X, br.Y);
    } else if(event.getType() == Texture::DESTROY) {
      textureID = NULL;
      return true;
    }

    Gsage::Vector2 texSize = mTexture->getSrcSize();
    mSize = ImVec2(texSize.X, texSize.Y);
    return true;
  }
}
