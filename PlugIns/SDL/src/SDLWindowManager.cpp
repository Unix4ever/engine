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

#include "SDLWindowManager.h"
#include "SDLRenderer.h"
#include "SDLCore.h"

#include "EngineEvent.h"
#include "Engine.h"

#if GSAGE_PLATFORM == GSAGE_APPLE
#include "OSXUtils.h"
#endif
#include <SDL_syswm.h>

namespace Gsage {

  int SDL_GetDisplayForPoint(SDL_Point p)
  {
    int displays = SDL_GetNumVideoDisplays();
    SDL_Point points[1] = {p};
    SDL_Rect displayBounds;
    for(int i = 0; i < displays; i++) {
      SDL_GetDisplayUsableBounds(i, &displayBounds);
      if(p.x < 0 || p.y < 0) {
        return i;
      }

      SDL_Rect r;
      if(SDL_EnclosePoints(&points[0], 1, &displayBounds, &r)) {
        return i;
      }
    }

    return 0;
  }

  float SDL_GetScaleFactor(SDL_Point p)
  {
    float ddpi, hdpi, vdpi;
    SDL_GetDisplayDPI(SDL_GetDisplayForPoint(p), &ddpi, &hdpi, &vdpi);
//#if GSAGE_PLATFORM == GSAGE_APPLE
//  TODO: fix DPI awareness on OSX
//    float defaultDPI = 72.0f;
//#else
    float defaultDPI = 96.0f;
//#endif
    return hdpi/defaultDPI;
  }

  SDLWindow::SDLWindow(const std::string& name, SDL_Window* window, SDLWindow::Cursors& cursors)
    : Window(name)
    , mWindow(window)
    , mGLContext(nullptr)
    , mCursors(cursors)
    , mTransparent(false)
  {
  }

  SDLWindow::~SDLWindow()
  {
    if(mGLContext) {
      SDL_GL_DeleteContext(mGLContext);
      LOG(INFO) << "GL Context destroyed";
    }
    SDL_DestroyWindow(mWindow);
  }

  unsigned long long SDLWindow::getWindowHandle()
  {
    unsigned long handle = 0;
    SDL_SysWMinfo windowInfo;
    SDL_VERSION(&windowInfo.version);
    if(SDL_GetWindowWMInfo(mWindow, &windowInfo) == SDL_FALSE) {
      LOG(ERROR) << "Failed to get window info";
      return 0;
    }
#if GSAGE_PLATFORM == GSAGE_APPLE
    handle = WindowContentViewHandle(windowInfo);
#elif GSAGE_PLATFORM == GSAGE_LINUX
    handle = (unsigned long long) windowInfo.info.x11.window;
#elif GSAGE_PLATFORM == GSAGE_WIN32
    handle = (unsigned long long) windowInfo.info.win.window;
#endif

    return handle;
  }

  std::tuple<int, int> SDLWindow::getPosition() const
  {
    int x, y;
    SDL_GetWindowPosition(mWindow, &x, &y);
    return std::make_tuple(x, y);
  }

  void SDLWindow::setPosition(int x, int y)
  {
    SDL_SetWindowPosition(mWindow, x, y);
  }

  std::tuple<int, int> SDLWindow::getSize()
  {
    int width = 0;
    int height = 0;
    SDL_GetWindowSize(mWindow, &width, &height);
    return std::make_tuple(width, height);
  }

  void SDLWindow::setSize(int width, int height)
  {
    SDL_SetWindowSize(mWindow, width, height);
  }

  std::tuple<int, int, int, int> SDLWindow::getDisplayBounds()
  {
    int index = getDisplay();
    SDL_Rect displayBounds;
    SDL_GetDisplayUsableBounds(index, &displayBounds);
#if GSAGE_PLATFORM != GSAGE_APPLE
    int top;
    int left;
    int bottom;
    int right;
    SDL_GetWindowBordersSize(mWindow, &top, &left, &bottom, &right);
    displayBounds.y += top;
    displayBounds.h -= top;
#endif

    return std::make_tuple(displayBounds.x, displayBounds.y, displayBounds.w, displayBounds.h);
  }

  void* SDLWindow::getGLContext()
  {
    if(mGLContext == nullptr) {
      return 0;
    }

    return (void*) mGLContext;
  }

  float SDLWindow::getScaleFactor() const
  {
    SDL_Point p = {0, 0};
    std::tie(p.x, p.y) = getPosition();
    return SDL_GetScaleFactor(p);
  }

  int SDLWindow::getDisplay() const
  {
    SDL_Point p = {0, 0};
    std::tie(p.x, p.y) = getPosition();
    return SDL_GetDisplayForPoint(p);
  }

  void SDLWindow::show()
  {
    SDL_ShowWindow(mWindow);
  }

  void SDLWindow::hide()
  {
    SDL_HideWindow(mWindow);
  }

  void SDLWindow::setCursor(Window::Cursor c)
  {
    if(c >= mCursors.size()) {
      c = Window::Cursor::Arrow;
    }

    if(c < 0) {
      SDL_ShowCursor(SDL_FALSE);
      return;
    }

    SDL_Cursor* cursor = mCursors[c];
    if(cursor) {
      SDL_SetCursor(cursor);
      SDL_ShowCursor(SDL_TRUE);
    }
  }

  void SDLWindow::exposed()
  {
    if(mTransparent) {
      SDL_SysWMinfo windowInfo;
      SDL_VERSION(&windowInfo.version);
      if(SDL_GetWindowWMInfo(mWindow, &windowInfo) == SDL_FALSE) {
        LOG(ERROR) << "Failed to get window info " << mWindow;
        return;
      }
#if GSAGE_PLATFORM == GSAGE_APPLE
      MakeWindowTransparent(windowInfo);
#endif
    }
  }

  SDLWindowManager::SDLWindowManager(const std::string& type, SDLCore* core)
    : WindowManager(type)
    , mCore(core)
  {
    mCore->setWindowManager(this);
  }

  SDLWindowManager::~SDLWindowManager()
  {
    while(mCursors.size() > 0) {
      SDL_FreeCursor(mCursors[mCursors.size() - 1]);
      mCursors.pop_back();
    }
    mCore->setWindowManager(nullptr);
  }

  bool SDLWindowManager::initialize(const DataProxy& config)
  {
    SDL_SetHintWithPriority(SDL_HINT_MOUSE_FOCUS_CLICKTHROUGH, "1", SDL_HINT_OVERRIDE);
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    int initializationCode = SDL_InitSubSystem(SDL_INIT_VIDEO);
    if(initializationCode < 0) {
      LOG(ERROR) << "Failed to initialize video subsystem " << SDL_GetError();
      return false;
    }

    mCursors.resize(8);
    mCursors[Window::Arrow] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    mCursors[Window::TextInput] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    mCursors[Window::ResizeAll] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    mCursors[Window::ResizeNS] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    mCursors[Window::ResizeEW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    mCursors[Window::ResizeNESW] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    mCursors[Window::ResizeNWSE] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    mCursors[Window::Hand] = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    auto windows = config.get<DataProxy>("windows");
    if(!windows.second) {
      return true;
    }

    for(auto pair : windows.first) {
      LOG(TRACE) << "Creating window " << pair.first;
      createWindow(pair.first, pair.second.get("width", 320), pair.second.get("height", 240), pair.second.get("fullscreen", false), pair.second);
    }
    return true;
  }

  WindowPtr SDLWindowManager::createWindow(const std::string& name, unsigned int width, unsigned int height,
          bool fullscreen, const DataProxy& params)
  {
    Uint32 flags = 0;
    int x = params.get("x", SDL_WINDOWPOS_UNDEFINED);
    int y = params.get("y", SDL_WINDOWPOS_UNDEFINED);
    bool showCursor = params.get("showCursor", true);

    std::map<std::string, Uint32> flagsMap;
    flagsMap["hidden"] = SDL_WINDOW_HIDDEN;
    flagsMap["borderless"] = SDL_WINDOW_BORDERLESS;
    flagsMap["openGL"] = SDL_WINDOW_OPENGL;
    flagsMap["resizable"] = SDL_WINDOW_RESIZABLE;
    flagsMap["minimized"] = SDL_WINDOW_MINIMIZED;
    flagsMap["maximized"] = SDL_WINDOW_MAXIMIZED;
    flagsMap["inputGrabbed"] = SDL_WINDOW_INPUT_GRABBED;
    flagsMap["highDPI"] = SDL_WINDOW_ALLOW_HIGHDPI;

    for(auto pair : flagsMap) {
      if(params.get(pair.first, false)) {
        flags |= pair.second;
      }
    }

    if(fullscreen) {
      flags |= SDL_WINDOW_FULLSCREEN;
    }

#if GSAGE_PLATFORM == GSAGE_WIN32 || GSAGE_PLATFORM == GSAGE_LINUX
    SDL_Point position = {x, y};
    float factor = SDL_GetScaleFactor(position);
#else
    float factor = 1.0f;
#endif

    SDL_Window* window = SDL_CreateWindow(name.c_str(), x, y, width * factor, height * factor, flags);
    if(window == NULL) {
      LOG(ERROR) << "Failed to create window " << SDL_GetError();
      return nullptr;
    }

    SDL_GLContext sdlContext = nullptr;
    if((flags & SDL_WINDOW_OPENGL) != 0) {
      sdlContext = SDL_GL_CreateContext(window);
      if(!sdlContext) {
        LOG(ERROR) << "Failed to create SDL GL context " <<  SDL_GetError();
        SDL_DestroyWindow(window);
        return nullptr;
      }
    }

    SDLWindow* wrapper = new SDLWindow(name, window, mCursors);
    if(sdlContext) {
      wrapper->mGLContext = sdlContext;
    }
    wrapper->mTransparent = params.get("transparent", false);
    SDL_SysWMinfo windowInfo;
    SDL_VERSION(&windowInfo.version);
    if(SDL_GetWindowWMInfo(window, &windowInfo) == SDL_FALSE) {
      LOG(ERROR) << "Failed to get window info " << window;
      return 0;
    }
#if GSAGE_PLATFORM == GSAGE_APPLE
    AdjustWindowSettings(windowInfo);
#endif
    if(wrapper->mTransparent) {
#if GSAGE_PLATFORM == GSAGE_APPLE
      MakeWindowTransparent(windowInfo);
#endif
    }

    SDL_Renderer* renderer = nullptr;

    mWindowsMutex.lock();
    WindowPtr res(wrapper);
    windowCreated(res, params.get("render", false));

    auto rendererParams = params.get<DataProxy>("renderer");
    if(rendererParams.second) {
      renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
      mRenderers[name] = std::move(std::make_unique<SDLRenderer>(res, rendererParams.first, renderer, mCore));
    }

    mWindowsByID[wrapper->getWindowID()] = res;
    mWindowsMutex.unlock();
    return res;
  }

  bool SDLWindowManager::destroyWindow(WindowPtr window)
  {
    mWindowsMutex.lock();
    bool res = windowDestroyed(window);
    // delete window renderer if any
    if(res) {
      if(contains(mRenderers, window->getName())) {
        mRenderers.erase(window->getName());
      }

      Uint32 id = static_cast<SDLWindow*>(window.get())->getWindowID();
      GSAGE_ASSERT(contains(mWindowsByID, id), "detected inconsistent windows state");
      mWindowsByID.erase(id);
    }
    mWindowsMutex.unlock();
    return res;
  }

  void SDLWindowManager::update(double time)
  {
    for(auto& pair : mRenderers) {
      pair.second->render();
    }
  }

}
