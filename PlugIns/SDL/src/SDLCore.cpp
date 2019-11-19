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

#include "SDLCore.h"
#include <SDL2/SDL.h>
#include "GsageFacade.h"
#include "EngineEvent.h"
#include "SDLWindowManager.h"

namespace Gsage {

  SDLCore::SDLCore()
    : mWindowManager(nullptr)
    , mFacade(nullptr)
  {
  }

  SDLCore::~SDLCore()
  {
  }

  bool SDLCore::initialize(const DataProxy& params, GsageFacade* facade)
  {
    if(SDL_Init(0) < 0) {
      LOG(ERROR) << "Failed to initialize SDL " << SDL_GetError();
      return false;
    }
    mInitialized = true;
    mFacade = facade;
    facade->addUpdateListener(this);
    return true;
  }

  void SDLCore::update(double time)
  {
    SDL_Event event;
    while(SDL_PollEvent(&event) != 0) {
      if(event.type == SDL_QUIT) {
        mFacade->shutdown();
        return;
      }

      WindowPtr window;

      switch(event.type) {
        case SDL_WINDOWEVENT:
          window = mWindowManager->getWindowByID(event.window.windowID);
          if(!window) {
            break;
          }
          switch(event.window.event){
            case SDL_WINDOWEVENT_SIZE_CHANGED:
              mFacade->getEngine()->fireEvent(WindowEvent(WindowEvent::RESIZE, window->getWindowHandle(), window->getName().c_str(), event.window.data1, event.window.data2));
              break;
            case SDL_WINDOWEVENT_MOVED:
              mFacade->getEngine()->fireEvent(WindowEvent(WindowEvent::MOVE, window->getWindowHandle(), window->getName().c_str(), 0, 0, event.window.data1, event.window.data2));
              break;
          }
          break;
        case SDL_DROPBEGIN:
          mFacade->getEngine()->fireEvent(DropFileEvent(DropFileEvent::DROP_BEGIN, ""));
          break;
        case SDL_DROPFILE:
          mFacade->getEngine()->fireEvent(DropFileEvent(DropFileEvent::DROP_FILE, event.drop.file));
          SDL_free(event.drop.file);
          break;
      }

      for(auto listener : mEventListeners) {
        listener->handleEvent(&event);
      }
    }

    if(mWindowManager) {
      mWindowManager->update(time);
    }
  }

  void SDLCore::tearDown()
  {
    SDL_Quit();
    mInitialized = false;
  }

  void SDLCore::addEventListener(SDLEventListener* listener)
  {
    mEventListeners.push_back(listener);
  }

  void SDLCore::removeEventListener(SDLEventListener* listener)
  {
    mEventListeners.erase(std::remove(mEventListeners.begin(), mEventListeners.end(), listener), mEventListeners.end());
  }

  void SDLCore::setWindowManager(SDLWindowManager* value)
  {
    mWindowManager = value;
  }

  const std::string& SDLCore::getResourcePath() const {
    return mFacade->getResourcePath();
  }
}
