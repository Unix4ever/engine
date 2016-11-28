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

#include "RocketUIManager.h"

#include <Rocket/Core/Lua/Interpreter.h>
#include <Rocket/Controls/Lua/Controls.h>
#include "RenderInterfaceOgre3D.h"
#include "SystemInterfaceOgre3D.h"

#include "MouseEvent.h"
#include "RenderEvent.h"
#include "Engine.h"

#include "systems/OgreRenderSystem.h"

namespace Gsage {

  RocketUIManager::RocketUIManager()
    : mRenderInterface(0)
    , mSystemInterface(0)
    , mContext(0)
  {
    buildKeyMap();
  }

  RocketUIManager::~RocketUIManager()
  {
    if(mContext)
      mContext->RemoveReference();
    Rocket::Core::Shutdown();

    if(mRenderInterface)
      delete mRenderInterface;

    if(mSystemInterface)
      delete mSystemInterface;
  }

  void RocketUIManager::initialize(Engine* engine, lua_State* luaState)
  {
    UIManager::initialize(engine, luaState);
    if(engine->hasSystem("render"))
    {
      OgreRenderSystem* renderer = static_cast<OgreRenderSystem*>(engine->getSystem("render"));
      setUp(renderer->getWidth(), renderer->getHeight());
    }

    // add render handler
    addEventListener(mEngine, RenderEvent::UPDATE_UI, &RocketUIManager::render);
    // mouse events
    addEventListener(mEngine, MouseEvent::MOUSE_DOWN, &RocketUIManager::handleMouseEvent, -100);
    addEventListener(mEngine, MouseEvent::MOUSE_UP, &RocketUIManager::handleMouseEvent, -100);
    addEventListener(mEngine, MouseEvent::MOUSE_MOVE, &RocketUIManager::handleMouseEvent, -100);
    // keyboard events
    addEventListener(mEngine, KeyboardEvent::KEY_DOWN, &RocketUIManager::handleKeyboardEvent, -100);
    addEventListener(mEngine, KeyboardEvent::KEY_UP, &RocketUIManager::handleKeyboardEvent, -100);
  }

  lua_State* RocketUIManager::getLuaState()
  {
    return Rocket::Core::Lua::Interpreter::GetLuaState();
  }

  void RocketUIManager::setLuaState(lua_State* L)
  {
    mLuaState = L;
    // Initialise the Lua interface
    Rocket::Core::Lua::Interpreter::Initialise(mLuaState);
    Rocket::Controls::Lua::RegisterTypes(Rocket::Core::Lua::Interpreter::GetLuaState());
  }

  bool RocketUIManager::render(EventDispatcher* sender, const Event& event)
  {
    RenderEvent e = static_cast<const RenderEvent&>(event);
    if(!mRenderInterface || !mSystemInterface)
      setUp(e.getRenderSystem()->getWidth(),
            e.getRenderSystem()->getHeight());

    mContext->Update();
    configureRenderSystem(e);
    mContext->Render();
    return true;
  }

  void RocketUIManager::configureRenderSystem(RenderEvent& event)
  {
    OgreRenderSystem* gsageRendering = event.getRenderSystem();
    Ogre::RenderSystem* renderSystem = gsageRendering->getRenderSystem();

    // Set up the projection and view matrices.
    Ogre::Matrix4 projectionMatrix;
    float zNear = -1;
    float zFar = 1;

    projectionMatrix = Ogre::Matrix4::ZERO;

    // Set up matrices.
    projectionMatrix[0][0] = 2.0f / gsageRendering->getWidth();
    projectionMatrix[0][3]= -1.0000000f;
    projectionMatrix[1][1]= -2.0f / gsageRendering->getHeight();
    projectionMatrix[1][3]= 1.0000000f;
    projectionMatrix[2][2]= -2.0f / (zFar - zNear);
    projectionMatrix[3][3]= 1.0000000f;
    renderSystem->_setProjectionMatrix(projectionMatrix);
    renderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);

    // Disable lighting, as all of Rocket's geometry is unlit.
    renderSystem->setLightingEnabled(false);
    // Disable depth-buffering; all of the geometry is already depth-sorted.
    renderSystem->_setDepthBufferParams(false, false);
    // Rocket generates anti-clockwise geometry, so enable clockwise-culling.
    renderSystem->_setCullingMode(Ogre::CULL_CLOCKWISE);
    // Disable fogging.
    renderSystem->_setFog(Ogre::FOG_NONE);
    // Enable writing to all four channels.
    renderSystem->_setColourBufferWriteEnabled(true, true, true, true);
    // Unbind any vertex or fragment programs bound previously by the application.
    renderSystem->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
    renderSystem->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);

    // Set texture settings to clamp along both axes.
    Ogre::TextureUnitState::UVWAddressingMode addressingMode;
    addressingMode.u = Ogre::TextureUnitState::TAM_CLAMP;
    addressingMode.v = Ogre::TextureUnitState::TAM_CLAMP;
    addressingMode.w = Ogre::TextureUnitState::TAM_CLAMP;
    renderSystem->_setTextureAddressingMode(0, addressingMode);

    // Set the texture coordinates for unit 0 to be read from unit 0.
    renderSystem->_setTextureCoordSet(0, 0);
    // Disable texture coordinate calculation.
    renderSystem->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
    // Enable linear filtering; images should be rendering 1 texel == 1 pixel, so point filtering could be used
    // except in the case of scaling tiled decorators.
    renderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
    // Disable texture coordinate transforms.
    renderSystem->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
    // Reject pixels with an alpha of 0.
    renderSystem->_setAlphaRejectSettings(Ogre::CMPF_GREATER, 0, false);
    // Disable all texture units but the first.
    renderSystem->_disableTextureUnitsFrom(1);

    // Enable simple alpha blending.
    renderSystem->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);

    // Disable depth bias.
    renderSystem->_setDepthBias(0, 0);
  }

  void RocketUIManager::setUp(unsigned int width, unsigned int height)
  {
    mRenderInterface = new RenderInterfaceOgre3D(width, height);
    mSystemInterface = new SystemInterfaceOgre3D();

    Rocket::Core::SetRenderInterface(mRenderInterface);
    Rocket::Core::SetSystemInterface(mSystemInterface);

    Rocket::Core::Initialise();
    Rocket::Controls::Initialise();
    setLuaState(mLuaState);
    mContext = Rocket::Core::CreateContext("main", Rocket::Core::Vector2i(width, height));
  }

  bool RocketUIManager::handleMouseEvent(EventDispatcher* sender, const Event& event)
  {
    if(!mContext)
      return true;

    const MouseEvent& e = static_cast<const MouseEvent&>(event);

    if(e.getType() == MouseEvent::MOUSE_MOVE)
    {
      int keyModifierState = getKeyModifierState();

      mContext->ProcessMouseMove(e.mouseX, e.mouseY, keyModifierState);
      if (e.relativeZ != 0)
      {
        mContext->ProcessMouseWheel(e.relativeZ / -120, keyModifierState);
        return !doCapture();
      }
    }
    else if(e.getType() == MouseEvent::MOUSE_DOWN)
    {
      mContext->ProcessMouseButtonDown((int) e.button, getKeyModifierState());
      return !doCapture();
    }
    else if(e.getType() == MouseEvent::MOUSE_UP)
    {
      mContext->ProcessMouseButtonUp((int) e.button, getKeyModifierState());
    }
    return true;
  }

  bool RocketUIManager::handleKeyboardEvent(EventDispatcher* sender, const Event& event)
  {
    const KeyboardEvent& e = static_cast<const KeyboardEvent&>(event);
    Rocket::Core::Input::KeyIdentifier key = mKeyMap[e.key];
    mModifiersState = e.getModifiersState();

    if(e.getType() == KeyboardEvent::KEY_UP)
    {
      if(key != Rocket::Core::Input::KI_UNKNOWN)
        mContext->ProcessKeyUp(key, getKeyModifierState());
    }
    else if(e.getType() == KeyboardEvent::KEY_DOWN)
    {
      if (key != Rocket::Core::Input::KI_UNKNOWN)
        mContext->ProcessKeyDown(key, getKeyModifierState());

      // Send through the ASCII value as text input if it is printable.
      if (e.text >= 32)
        mContext->ProcessTextInput((Rocket::Core::word) e.text);
      else if (key == Rocket::Core::Input::KI_RETURN)
        mContext->ProcessTextInput((Rocket::Core::word) '\n');
    }
    return true;
  }

  int RocketUIManager::getKeyModifierState()
  {
    int modifierState = 0;
    if ((mModifiersState & KeyboardEvent::Modifier::Ctrl) == KeyboardEvent::Modifier::Ctrl)
      modifierState |= Rocket::Core::Input::KM_CTRL;
    if ((mModifiersState & KeyboardEvent::Modifier::Shift) == KeyboardEvent::Modifier::Shift)
      modifierState |= Rocket::Core::Input::KM_SHIFT;
    if ((mModifiersState & KeyboardEvent::Modifier::Alt) == KeyboardEvent::Modifier::Alt)
      modifierState |= Rocket::Core::Input::KM_ALT;
    return modifierState;
  /*
  TODO: this one is not compiling on linux (at least), need to figure out how to fix it
#if OGRE_PLATFORM == OGRE_PLATFORM_WIN32

  if (GetKeyState(VK_CAPITAL) > 0)
  modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;
  if (GetKeyState(VK_NUMLOCK) > 0)
  modifier_state |= Rocket::Core::Input::KM_NUMLOCK;
  if (GetKeyState(VK_SCROLL) > 0)
  modifier_state |= Rocket::Core::Input::KM_SCROLLLOCK;

#elif OGRE_PLATFORM == OGRE_PLATFORM_APPLE

  UInt32 key_modifiers = GetCurrentEventKeyModifiers();
  if (key_modifiers & (1 << alphaLockBit))
  modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;

#elif OGRE_PLATFORM == OGRE_PLATFORM_LINUX

  XKeyboardState keyboard_state;
  XGetKeyboardControl(DISPLAY, &keyboard_state);

  if (keyboard_state.led_mask & (1 << 0))
  modifier_state |= Rocket::Core::Input::KM_CAPSLOCK;
  if (keyboard_state.led_mask & (1 << 1))
  modifier_state |= Rocket::Core::Input::KM_NUMLOCK;
  if (keyboard_state.led_mask & (1 << 2))
  modifier_state |= Rocket::Core::Input::KM_SCROLLLOCK;

#endif

  return modifier_state;*/
  }

  void RocketUIManager::buildKeyMap()
  {
    mKeyMap[KeyboardEvent::KC_UNASSIGNED] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_ESCAPE] = Rocket::Core::Input::KI_ESCAPE;
    mKeyMap[KeyboardEvent::KC_1] = Rocket::Core::Input::KI_1;
    mKeyMap[KeyboardEvent::KC_2] = Rocket::Core::Input::KI_2;
    mKeyMap[KeyboardEvent::KC_3] = Rocket::Core::Input::KI_3;
    mKeyMap[KeyboardEvent::KC_4] = Rocket::Core::Input::KI_4;
    mKeyMap[KeyboardEvent::KC_5] = Rocket::Core::Input::KI_5;
    mKeyMap[KeyboardEvent::KC_6] = Rocket::Core::Input::KI_6;
    mKeyMap[KeyboardEvent::KC_7] = Rocket::Core::Input::KI_7;
    mKeyMap[KeyboardEvent::KC_8] = Rocket::Core::Input::KI_8;
    mKeyMap[KeyboardEvent::KC_9] = Rocket::Core::Input::KI_9;
    mKeyMap[KeyboardEvent::KC_0] = Rocket::Core::Input::KI_0;
    mKeyMap[KeyboardEvent::KC_MINUS] = Rocket::Core::Input::KI_OEM_MINUS;
    mKeyMap[KeyboardEvent::KC_EQUALS] = Rocket::Core::Input::KI_OEM_PLUS;
    mKeyMap[KeyboardEvent::KC_BACK] = Rocket::Core::Input::KI_BACK;
    mKeyMap[KeyboardEvent::KC_TAB] = Rocket::Core::Input::KI_TAB;
    mKeyMap[KeyboardEvent::KC_Q] = Rocket::Core::Input::KI_Q;
    mKeyMap[KeyboardEvent::KC_W] = Rocket::Core::Input::KI_W;
    mKeyMap[KeyboardEvent::KC_E] = Rocket::Core::Input::KI_E;
    mKeyMap[KeyboardEvent::KC_R] = Rocket::Core::Input::KI_R;
    mKeyMap[KeyboardEvent::KC_T] = Rocket::Core::Input::KI_T;
    mKeyMap[KeyboardEvent::KC_Y] = Rocket::Core::Input::KI_Y;
    mKeyMap[KeyboardEvent::KC_U] = Rocket::Core::Input::KI_U;
    mKeyMap[KeyboardEvent::KC_I] = Rocket::Core::Input::KI_I;
    mKeyMap[KeyboardEvent::KC_O] = Rocket::Core::Input::KI_O;
    mKeyMap[KeyboardEvent::KC_P] = Rocket::Core::Input::KI_P;
    mKeyMap[KeyboardEvent::KC_LBRACKET] = Rocket::Core::Input::KI_OEM_4;
    mKeyMap[KeyboardEvent::KC_RBRACKET] = Rocket::Core::Input::KI_OEM_6;
    mKeyMap[KeyboardEvent::KC_RETURN] = Rocket::Core::Input::KI_RETURN;
    mKeyMap[KeyboardEvent::KC_LCONTROL] = Rocket::Core::Input::KI_LCONTROL;
    mKeyMap[KeyboardEvent::KC_A] = Rocket::Core::Input::KI_A;
    mKeyMap[KeyboardEvent::KC_S] = Rocket::Core::Input::KI_S;
    mKeyMap[KeyboardEvent::KC_D] = Rocket::Core::Input::KI_D;
    mKeyMap[KeyboardEvent::KC_F] = Rocket::Core::Input::KI_F;
    mKeyMap[KeyboardEvent::KC_G] = Rocket::Core::Input::KI_G;
    mKeyMap[KeyboardEvent::KC_H] = Rocket::Core::Input::KI_H;
    mKeyMap[KeyboardEvent::KC_J] = Rocket::Core::Input::KI_J;
    mKeyMap[KeyboardEvent::KC_K] = Rocket::Core::Input::KI_K;
    mKeyMap[KeyboardEvent::KC_L] = Rocket::Core::Input::KI_L;
    mKeyMap[KeyboardEvent::KC_SEMICOLON] = Rocket::Core::Input::KI_OEM_1;
    mKeyMap[KeyboardEvent::KC_APOSTROPHE] = Rocket::Core::Input::KI_OEM_7;
    mKeyMap[KeyboardEvent::KC_GRAVE] = Rocket::Core::Input::KI_OEM_3;
    mKeyMap[KeyboardEvent::KC_LSHIFT] = Rocket::Core::Input::KI_LSHIFT;
    mKeyMap[KeyboardEvent::KC_BACKSLASH] = Rocket::Core::Input::KI_OEM_5;
    mKeyMap[KeyboardEvent::KC_Z] = Rocket::Core::Input::KI_Z;
    mKeyMap[KeyboardEvent::KC_X] = Rocket::Core::Input::KI_X;
    mKeyMap[KeyboardEvent::KC_C] = Rocket::Core::Input::KI_C;
    mKeyMap[KeyboardEvent::KC_V] = Rocket::Core::Input::KI_V;
    mKeyMap[KeyboardEvent::KC_B] = Rocket::Core::Input::KI_B;
    mKeyMap[KeyboardEvent::KC_N] = Rocket::Core::Input::KI_N;
    mKeyMap[KeyboardEvent::KC_M] = Rocket::Core::Input::KI_M;
    mKeyMap[KeyboardEvent::KC_COMMA] = Rocket::Core::Input::KI_OEM_COMMA;
    mKeyMap[KeyboardEvent::KC_PERIOD] = Rocket::Core::Input::KI_OEM_PERIOD;
    mKeyMap[KeyboardEvent::KC_SLASH] = Rocket::Core::Input::KI_OEM_2;
    mKeyMap[KeyboardEvent::KC_RSHIFT] = Rocket::Core::Input::KI_RSHIFT;
    mKeyMap[KeyboardEvent::KC_MULTIPLY] = Rocket::Core::Input::KI_MULTIPLY;
    mKeyMap[KeyboardEvent::KC_LMENU] = Rocket::Core::Input::KI_LMENU;
    mKeyMap[KeyboardEvent::KC_SPACE] = Rocket::Core::Input::KI_SPACE;
    mKeyMap[KeyboardEvent::KC_CAPITAL] = Rocket::Core::Input::KI_CAPITAL;
    mKeyMap[KeyboardEvent::KC_F1] = Rocket::Core::Input::KI_F1;
    mKeyMap[KeyboardEvent::KC_F2] = Rocket::Core::Input::KI_F2;
    mKeyMap[KeyboardEvent::KC_F3] = Rocket::Core::Input::KI_F3;
    mKeyMap[KeyboardEvent::KC_F4] = Rocket::Core::Input::KI_F4;
    mKeyMap[KeyboardEvent::KC_F5] = Rocket::Core::Input::KI_F5;
    mKeyMap[KeyboardEvent::KC_F6] = Rocket::Core::Input::KI_F6;
    mKeyMap[KeyboardEvent::KC_F7] = Rocket::Core::Input::KI_F7;
    mKeyMap[KeyboardEvent::KC_F8] = Rocket::Core::Input::KI_F8;
    mKeyMap[KeyboardEvent::KC_F9] = Rocket::Core::Input::KI_F9;
    mKeyMap[KeyboardEvent::KC_F10] = Rocket::Core::Input::KI_F10;
    mKeyMap[KeyboardEvent::KC_NUMLOCK] = Rocket::Core::Input::KI_NUMLOCK;
    mKeyMap[KeyboardEvent::KC_SCROLL] = Rocket::Core::Input::KI_SCROLL;
    mKeyMap[KeyboardEvent::KC_NUMPAD7] = Rocket::Core::Input::KI_7;
    mKeyMap[KeyboardEvent::KC_NUMPAD8] = Rocket::Core::Input::KI_8;
    mKeyMap[KeyboardEvent::KC_NUMPAD9] = Rocket::Core::Input::KI_9;
    mKeyMap[KeyboardEvent::KC_SUBTRACT] = Rocket::Core::Input::KI_SUBTRACT;
    mKeyMap[KeyboardEvent::KC_NUMPAD4] = Rocket::Core::Input::KI_4;
    mKeyMap[KeyboardEvent::KC_NUMPAD5] = Rocket::Core::Input::KI_5;
    mKeyMap[KeyboardEvent::KC_NUMPAD6] = Rocket::Core::Input::KI_6;
    mKeyMap[KeyboardEvent::KC_ADD] = Rocket::Core::Input::KI_ADD;
    mKeyMap[KeyboardEvent::KC_NUMPAD1] = Rocket::Core::Input::KI_1;
    mKeyMap[KeyboardEvent::KC_NUMPAD2] = Rocket::Core::Input::KI_2;
    mKeyMap[KeyboardEvent::KC_NUMPAD3] = Rocket::Core::Input::KI_3;
    mKeyMap[KeyboardEvent::KC_NUMPAD0] = Rocket::Core::Input::KI_0;
    mKeyMap[KeyboardEvent::KC_DECIMAL] = Rocket::Core::Input::KI_DECIMAL;
    mKeyMap[KeyboardEvent::KC_OEM_102] = Rocket::Core::Input::KI_OEM_102;
    mKeyMap[KeyboardEvent::KC_F11] = Rocket::Core::Input::KI_F11;
    mKeyMap[KeyboardEvent::KC_F12] = Rocket::Core::Input::KI_F12;
    mKeyMap[KeyboardEvent::KC_F13] = Rocket::Core::Input::KI_F13;
    mKeyMap[KeyboardEvent::KC_F14] = Rocket::Core::Input::KI_F14;
    mKeyMap[KeyboardEvent::KC_F15] = Rocket::Core::Input::KI_F15;
    mKeyMap[KeyboardEvent::KC_KANA] = Rocket::Core::Input::KI_KANA;
    mKeyMap[KeyboardEvent::KC_ABNT_C1] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_CONVERT] = Rocket::Core::Input::KI_CONVERT;
    mKeyMap[KeyboardEvent::KC_NOCONVERT] = Rocket::Core::Input::KI_NONCONVERT;
    mKeyMap[KeyboardEvent::KC_YEN] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_ABNT_C2] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_NUMPADEQUALS] = Rocket::Core::Input::KI_OEM_NEC_EQUAL;
    mKeyMap[KeyboardEvent::KC_PREVTRACK] = Rocket::Core::Input::KI_MEDIA_PREV_TRACK;
    mKeyMap[KeyboardEvent::KC_AT] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_COLON] = Rocket::Core::Input::KI_OEM_1;
    mKeyMap[KeyboardEvent::KC_UNDERLINE] = Rocket::Core::Input::KI_OEM_MINUS;
    mKeyMap[KeyboardEvent::KC_KANJI] = Rocket::Core::Input::KI_KANJI;
    mKeyMap[KeyboardEvent::KC_STOP] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_AX] = Rocket::Core::Input::KI_OEM_AX;
    mKeyMap[KeyboardEvent::KC_UNLABELED] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_NEXTTRACK] = Rocket::Core::Input::KI_MEDIA_NEXT_TRACK;
    mKeyMap[KeyboardEvent::KC_NUMPADENTER] = Rocket::Core::Input::KI_NUMPADENTER;
    mKeyMap[KeyboardEvent::KC_RCONTROL] = Rocket::Core::Input::KI_RCONTROL;
    mKeyMap[KeyboardEvent::KC_MUTE] = Rocket::Core::Input::KI_VOLUME_MUTE;
    mKeyMap[KeyboardEvent::KC_CALCULATOR] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_PLAYPAUSE] = Rocket::Core::Input::KI_MEDIA_PLAY_PAUSE;
    mKeyMap[KeyboardEvent::KC_MEDIASTOP] = Rocket::Core::Input::KI_MEDIA_STOP;
    mKeyMap[KeyboardEvent::KC_VOLUMEDOWN] = Rocket::Core::Input::KI_VOLUME_DOWN;
    mKeyMap[KeyboardEvent::KC_VOLUMEUP] = Rocket::Core::Input::KI_VOLUME_UP;
    mKeyMap[KeyboardEvent::KC_WEBHOME] = Rocket::Core::Input::KI_BROWSER_HOME;
    mKeyMap[KeyboardEvent::KC_NUMPADCOMMA] = Rocket::Core::Input::KI_SEPARATOR;
    mKeyMap[KeyboardEvent::KC_DIVIDE] = Rocket::Core::Input::KI_DIVIDE;
    mKeyMap[KeyboardEvent::KC_SYSRQ] = Rocket::Core::Input::KI_SNAPSHOT;
    mKeyMap[KeyboardEvent::KC_RMENU] = Rocket::Core::Input::KI_RMENU;
    mKeyMap[KeyboardEvent::KC_PAUSE] = Rocket::Core::Input::KI_PAUSE;
    mKeyMap[KeyboardEvent::KC_HOME] = Rocket::Core::Input::KI_HOME;
    mKeyMap[KeyboardEvent::KC_UP] = Rocket::Core::Input::KI_UP;
    mKeyMap[KeyboardEvent::KC_PGUP] = Rocket::Core::Input::KI_PRIOR;
    mKeyMap[KeyboardEvent::KC_LEFT] = Rocket::Core::Input::KI_LEFT;
    mKeyMap[KeyboardEvent::KC_RIGHT] = Rocket::Core::Input::KI_RIGHT;
    mKeyMap[KeyboardEvent::KC_END] = Rocket::Core::Input::KI_END;
    mKeyMap[KeyboardEvent::KC_DOWN] = Rocket::Core::Input::KI_DOWN;
    mKeyMap[KeyboardEvent::KC_PGDOWN] = Rocket::Core::Input::KI_NEXT;
    mKeyMap[KeyboardEvent::KC_INSERT] = Rocket::Core::Input::KI_INSERT;
    mKeyMap[KeyboardEvent::KC_DELETE] = Rocket::Core::Input::KI_DELETE;
    mKeyMap[KeyboardEvent::KC_LWIN] = Rocket::Core::Input::KI_LWIN;
    mKeyMap[KeyboardEvent::KC_RWIN] = Rocket::Core::Input::KI_RWIN;
    mKeyMap[KeyboardEvent::KC_APPS] = Rocket::Core::Input::KI_APPS;
    mKeyMap[KeyboardEvent::KC_POWER] = Rocket::Core::Input::KI_POWER;
    mKeyMap[KeyboardEvent::KC_SLEEP] = Rocket::Core::Input::KI_SLEEP;
    mKeyMap[KeyboardEvent::KC_WAKE] = Rocket::Core::Input::KI_WAKE;
    mKeyMap[KeyboardEvent::KC_WEBSEARCH] = Rocket::Core::Input::KI_BROWSER_SEARCH;
    mKeyMap[KeyboardEvent::KC_WEBFAVORITES] = Rocket::Core::Input::KI_BROWSER_FAVORITES;
    mKeyMap[KeyboardEvent::KC_WEBREFRESH] = Rocket::Core::Input::KI_BROWSER_REFRESH;
    mKeyMap[KeyboardEvent::KC_WEBSTOP] = Rocket::Core::Input::KI_BROWSER_STOP;
    mKeyMap[KeyboardEvent::KC_WEBFORWARD] = Rocket::Core::Input::KI_BROWSER_FORWARD;
    mKeyMap[KeyboardEvent::KC_WEBBACK] = Rocket::Core::Input::KI_BROWSER_BACK;
    mKeyMap[KeyboardEvent::KC_MYCOMPUTER] = Rocket::Core::Input::KI_UNKNOWN;
    mKeyMap[KeyboardEvent::KC_MAIL] = Rocket::Core::Input::KI_LAUNCH_MAIL;
    mKeyMap[KeyboardEvent::KC_MEDIASELECT] = Rocket::Core::Input::KI_LAUNCH_MEDIA_SELECT;
  }

  bool RocketUIManager::doCapture()
  {
    Rocket::Core::Element* e = mContext->GetHoverElement();

    if(e && e != mContext->GetRootElement())
    {
      if(e->GetTagName() == "body")
        return false;

      bool isVisible = true;
      while(e && (isVisible = e->IsVisible()))
      {
        e = e->GetParentNode();
      }

      return isVisible;
    }

    return false;
  }
}
