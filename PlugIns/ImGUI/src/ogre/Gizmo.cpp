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

#include "ogre/Gizmo.h"
#include "ogre/SceneNodeWrapper.h"
#include "systems/OgreRenderSystem.h"

#include "RenderTarget.h"

#include <imgui.h>

namespace Gsage {

  Gizmo::Gizmo(OgreRenderSystem* rs)
    : mRenderSystem(rs)
    , mTarget(nullptr)
    , mEnabled(false)
    , mOperation(ImGuizmo::TRANSLATE)
    , mMode(ImGuizmo::LOCAL)
  {
  }

  Gizmo::~Gizmo()
  {
  }

  void Gizmo::render(float x, float y, const std::string& rtName)
  {
    if(mTarget == nullptr || !mEnabled) {
      return;
    }

    RenderTargetPtr renderTarget = mRenderSystem->getRenderTarget(rtName);
    if(renderTarget == nullptr) {
      return;
    }

    Ogre::Camera* cam = renderTarget->getCamera();
    Ogre::Matrix4 view = cam->getViewMatrix();
    Ogre::Matrix4 projection = cam->getProjectionMatrix();

    float v[16] = {0.0f};
    float p[16] = {0.0f};

    if(!extractMatrix(view, v, 16))
      return;

    if(!extractMatrix(projection, p, 16))
      return;

    Ogre::SceneNode* node = mTarget->getNode();
    float matrixTranslation[3], matrixRotation[3], matrixScale[3];
    Ogre::Matrix4 model;

    Ogre::Vector3 position = node->_getDerivedPosition();
    Ogre::Vector3 scale = node->_getDerivedScale();
    Ogre::Quaternion orientation = node->_getDerivedOrientation();

    model.makeTransform(position, scale, orientation);

    if(!extractMatrix(model, &mModelMatrix[0], 16))
      return;

    ImGuizmo::BeginFrame();
    ImGuizmo::Enable(mEnabled);
    ImGuizmo::SetRect(x, y, renderTarget->getWidth(), renderTarget->getHeight());
    float delta[16] = {0.f};

    ImGuizmo::Manipulate(&v[0], &p[0], mOperation, mMode, &mModelMatrix[0], &delta[0]);

    if(!ImGuizmo::IsUsing()) {
      return;
    }

    ImGuizmo::DecomposeMatrixToComponents(mModelMatrix, matrixTranslation, matrixRotation, matrixScale);
    float deltaTransformation[3], deltaScale[3], deltaRotation[3];
    ImGuizmo::DecomposeMatrixToComponents(delta, deltaTransformation, deltaRotation, deltaScale);
    if(mOperation == ImGuizmo::TRANSLATE)
      node->setPosition(Ogre::Vector3(&matrixTranslation[0]));
    else if(mOperation == ImGuizmo::SCALE)
      node->setScale(Ogre::Vector3(&matrixScale[0]));
    else if(mOperation == ImGuizmo::ROTATE)
    {
      node->rotate(Ogre::Vector3::UNIT_X, Ogre::Degree(deltaRotation[0]), Ogre::SceneNode::TS_WORLD);
      node->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(deltaRotation[1]), Ogre::SceneNode::TS_WORLD);
      node->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(deltaRotation[2]), Ogre::SceneNode::TS_WORLD);
    }
    ImGuizmo::EndFrame();
  }

  void Gizmo::enable(bool value)
  {
    mEnabled = value;
  }

  void Gizmo::setTarget(SceneNodeWrapper* target)
  {
    mTarget = target;
  }

  void Gizmo::setOperation(ImGuizmo::OPERATION value)
  {
    mOperation = value;
  }

  ImGuizmo::OPERATION Gizmo::getOperation() const
  {
    return mOperation;
  }

  void Gizmo::setMode(ImGuizmo::MODE value)
  {
    mMode = value;
  }

  ImGuizmo::MODE Gizmo::getMode() const
  {
    return mMode;
  }

  bool Gizmo::drawCoordinatesEditor(float v_speed, float v_min, float v_max, const std::string& display_format, float power)
  {
    if(mTarget == nullptr || !mEnabled) {
      return false;
    }

    Ogre::SceneNode* node = mTarget->getNode();
    if(!node) {
      return false;
    }

    Ogre::Vector3 position = node->_getDerivedPosition();
    Ogre::Vector3 scale = node->_getDerivedScale();
    Ogre::Quaternion orientation = node->_getDerivedOrientation();

    float pitch = orientation.getPitch().valueDegrees();
    float yaw = orientation.getYaw().valueDegrees();
    float roll = orientation.getRoll().valueDegrees();
    float rotation[3] = {pitch, yaw, roll};

    ImGui::DragFloat3("position", position.ptr(), v_speed, v_min, v_max, display_format.c_str(), power);
    ImGui::DragFloat3("rotation", rotation, v_speed, v_min, v_max, display_format.c_str(), power);
    ImGui::DragFloat3("scale", scale.ptr(), v_speed, v_min, v_max, display_format.c_str(), power);

    node->setPosition(position);
    node->setScale(scale);

    float deltaPitch = rotation[0] - pitch;
    float deltaYaw = rotation[1] - yaw;
    float deltaRoll = rotation[2] - roll;

    if(deltaPitch) {
      node->rotate(Ogre::Vector3::UNIT_X, Ogre::Degree(deltaPitch), Ogre::SceneNode::TS_WORLD);
    }

    if(deltaYaw) {
      node->rotate(Ogre::Vector3::UNIT_Y, Ogre::Degree(deltaYaw), Ogre::SceneNode::TS_WORLD);
    }

    if(deltaRoll) {
      node->rotate(Ogre::Vector3::UNIT_Z, Ogre::Degree(deltaRoll), Ogre::SceneNode::TS_WORLD);
    }

    return true;
  }

  bool Gizmo::extractMatrix(Ogre::Matrix4 matrix, float* dest, int length)
  {
    if(length != 16) {
      return false;
    }

    float res[16] = {0.0f};
    int k = 0;
    for(int i = 0; i < 4; i++) {
      for(int j = 0; j < 4; j++) {
        res[k++] = matrix[j][i];
      }
    }
    memcpy(dest, res, sizeof(float)*16);
    return true;
  }
}
