#ifndef _ImguiViewport_H_
#define _ImguiViewport_H_

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

#include "systems/RenderSystem.h"
#include "EventSubscriber.h"
#include "imvue_element.h"
#include "ImguiImage.h"

namespace Gsage {
  /**
   * Renders viewport into the texture and displays it as an ImGui element
   */
  class ImguiViewport : public ImVue::Element, public EventSubscriber<ImguiViewport>
  {
    public:
      ImguiViewport();
      ~ImguiViewport();

      void renderBody();

      ImU32 bgColour;
      char* textureID;
    private:

      bool onTextureEvent(EventDispatcher* sender, const Event& event);

      void setTexture(TexturePtr texture);

      bool mDirty;

      char* mTextureID;
      TextureHandle mTextureHandle;
      TexturePtr mTexture;
      RenderSystem* mRender;
      ImVec2 mPosition;
      ImVec2 mUV0;
      ImVec2 mUV1;
      ImVec2 mSize;
  };
}

#endif
