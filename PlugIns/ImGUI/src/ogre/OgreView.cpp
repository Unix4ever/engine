/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2017 Artem Chernyshev

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

#include <OgreRectangle2D.h>

#include "ogre/OgreView.h"

#include "ImguiManager.h"

#include "systems/OgreRenderSystem.h"
#include "ogre/ImguiOgreWrapper.h"

#include "RenderTarget.h"

namespace Gsage {

  float convertCoordinate(const float coord, const float size)
  {
    return (coord / size) * 2 - 1;
  }

  OgreView::OgreView(OgreRenderSystem* render)
    : mRender(render)
    , mWidth(1)
    , mHeight(1)
    , mRect(new Ogre::Rectangle2D(true))
  {
    mRect->setBoundingBox(Ogre::AxisAlignedBox::BOX_INFINITE);
  }

  OgreView::~OgreView()
  {
    delete mRect;
  }

  void OgreView::render(unsigned int width, unsigned int height)
  {
    bool widthChange = mWidth != width;
    bool heightChange = mHeight != height;
    mWidth = width;
    mHeight = height;

    if(mWidth <= 0 || mHeight <= 0)
      return;

    if(mTextureID.empty())
      return;

    RenderTargetPtr renderTarget = mRender->getRenderTarget(mTextureID);

    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) {
      io.WantCaptureMouse = !renderTarget->isMouseOver();
    }

    if(widthChange || heightChange) {
      renderTarget->setDimensions(mWidth, mHeight);
    }

    ImVec2 pos = ImGui::GetCursorScreenPos();
		ImVec2 size = ImGui::GetIO().DisplaySize;

    mPosition = pos;
    renderTarget->setPosition(pos.x, pos.y);

    // inverting
    pos.y = size.y - pos.y;

    mRect->setCorners(
        convertCoordinate(pos.x, size.x),
        convertCoordinate(pos.y, size.y),
        convertCoordinate(pos.x + mWidth, size.x),
        convertCoordinate(pos.y - mHeight, size.y)
    );

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddCallback(NULL, mRect);
    if(widthChange || heightChange) {
      setTextureID(mTextureID);
    }
  }

  void OgreView::setTextureID(const std::string& textureID)
  {
    mTextureID = textureID;
    std::string materialID = mTextureID + "material";

    Ogre::MaterialManager& materialManager = Ogre::MaterialManager::getSingleton();
    Ogre::MaterialPtr renderMaterial = materialManager.getByName(
        materialID,
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

    if(renderMaterial.isNull()) {
      renderMaterial = materialManager.create(
          materialID,
          Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
      renderMaterial->getTechnique(0)->getPass(0)->setLightingEnabled(false);
      mRect->setMaterial(materialID);
    } else {
      renderMaterial->getTechnique(0)->getPass(0)->removeTextureUnitState(0);
    }

    renderMaterial->getTechnique(0)->getPass(0)->createTextureUnitState(textureID);
  }
}
