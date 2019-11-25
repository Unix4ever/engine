#ifndef _InputFactory_H_
#define _InputFactory_H_

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

#include "EventSubscriber.h"
#include "UpdateListener.h"
#include "MouseEvent.h"

namespace Gsage {
  class Engine;
  /**
   * Input interface
   */
  class InputHandler : public EventSubscriber<InputHandler>, public UpdateListener
  {
    public:
      InputHandler(size_t windowHandle, Engine* engine);
      virtual ~InputHandler() {};
      /**
       * Handle window resize
       *
       * @param width Window width
       * @param height Window height
       */
      virtual void handleResize(unsigned int width, unsigned int height) = 0;

      /**
       * Handle window close
       */
      virtual void handleClose() = 0;
    protected:
      bool handleWindowEvent(EventDispatcher* sender, const Event& event);

      virtual void fireMouseEvent(Event::ConstType type, int x, int y, int z);

      virtual void fireMouseEvent(Event::ConstType type, int x, int y, int z, const MouseEvent::ButtonType button);

      size_t mHandle;
      int mWidth;
      int mHeight;
      int mPreviousX;
      int mPreviousY;
      int mPreviousZ;
      Engine* mEngine;
  };

  /**
   * Input handler wrapped into shared pointer
   */
  typedef std::shared_ptr<InputHandler> InputHandlerPtr;

  /**
   * Base class for any input factory
   */
  class AbstractInputFactory
  {
    public:
      enum Capabilities {
        // use one single global input handler
        Global    = 1 << 0,
        // use handler per window
        PerWindow = 1 << 1
      };

      AbstractInputFactory() {};
      virtual ~AbstractInputFactory() {};
      /**
       * Creates input handler
       *
       * @param windowHandle Window handle
       * @param engine Gsage::Engine pointer
       *
       * @returns pointer to handler. It should be wrapped into smart pointer
       */
      virtual InputHandlerPtr create(size_t windowHandle, Engine* engine) = 0;

      /**
       * Input factory capabilities
       */
      virtual Capabilities getCapabilities() const {
        return Global;
      }
  };

  /**
   * Input manager creates input handler for each new window.
   * Type of input can be configured by passing any concrete factory to the constructor.
   */
  class InputManager : public EventSubscriber<InputManager>, public UpdateListener
  {
    public:
      /**
       * @param engine Pointer to the engine
       */
      InputManager(Engine* engine);

      virtual ~InputManager();

      /**
       * Tells InputFactory to use any particular factory
       * @param id String id of factory
       */
      void useFactory(const std::string& id);

      /**
       * Register new type of input factory
       *
       * @param id String id of factory
       */
      template<class T>
      T* registerFactory(const std::string& id)
      {
        if(!std::is_base_of<AbstractInputFactory, T>::value)
        {
          LOG(ERROR) << "Factory for id '" << id << "' was not registered. It must be derived from AbstractInputFactory";
          return 0;
        }
        T* factory = new T();
        mFactories[id] = factory;
        if(mCurrentFactoryId == id) {
          useFactory(id);
        }
        return factory;
      }

      /**
       * Remove factory by id. It will also remove all input listeners
       *
       * @param id String id of factory
       */
      void removeFactory(const std::string& id);

      /**
       * Update all handlers
       *
       * @param time Elapsed time
       */
      void update(double time);

      /**
       * Get current factory ID
       */
      inline const std::string& getCurrentFactoryId() const { return mCurrentFactoryId; }

    private:
      bool handleWindowEvent(EventDispatcher* sender, const Event& e);
      Engine* mEngine;

      typedef std::map<size_t, InputHandlerPtr> InputHandlers;
      InputHandlers mInputHandlers;

      typedef std::map<std::string, AbstractInputFactory*> InputFactories;
      InputFactories mFactories;

      std::string mCurrentFactoryId;
      AbstractInputFactory* mCurrentFactory;
  };
}

#endif
