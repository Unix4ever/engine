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

#ifndef _EventDispatcher_H_
#define _EventDispatcher_H_

#include <algorithm>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <functional>
#include <mutex>
#include <cstring>
#include "GsageDefinitions.h"

namespace Gsage {
  class EventDispatcher;
  class EventSignal;
  class Event;

  /**
   * std function that represents event callback
   */
  typedef std::function<bool (EventDispatcher*, const Event&)> EventCallback;

  typedef std::shared_ptr<EventSignal> EventSignalPtr;

  /**
   * Identifies single connection to the EventSignal
   * - signal pointer
   * - priority int
   * - id int
   */
  class EventConnection
  {
    public:
      EventConnection(EventSignalPtr signal, int priority, int id);
      virtual ~EventConnection();
      /**
       * Disconnect underlying connection
       */
      void disconnect();
    private:
      EventSignalPtr mSignal;
      int mPriority;
      int mId;
  };

  /**
   * Lightweight version of boost::signal2
   */
  class EventSignal
  {
    public:
      EventSignal();
      virtual ~EventSignal();
      /**
       * Connect to signal
       * @param priority 0 means the biggest priority
       * @param callback function to call
       */
      int connect(const int priority, EventCallback callback);

      /**
       * Call signall
       * @param dispatcher pointer to dispatcher which calls this signal
       * @param event to pass
       *
       * @returns count of handlers that processed response
       */
      int operator()(EventDispatcher* dispatcher, const Event& event);

      /**
       *Disconnect one callback identified by priority and id
       */
      void disconnect(int priority, int id);
    private:
      typedef std::map<int, EventCallback> CallbacksList;

      typedef std::map<int, CallbacksList> Connections;

      Connections mConnections;
  };

  /**
   * Abstract event class
   */
  class GSAGE_API Event
  {
    public:
      typedef char const* Type;
      typedef const char* ConstType;

      Event() {}
      Event(ConstType type) : mType(type) {}
      virtual ~Event() {}

      /**
       * Get event type
       */
      ConstType getType() const { return mType; };

      /**
       * Check event type
       */
      bool is(ConstType type) const { return std::strcmp(mType, type) == 0; }
    private:
      ConstType mType;

  };

  /**
   * Const char comparison function for std::map
   */
  struct CmpEventType
  {
     bool operator()(Event::ConstType a, Event::ConstType b) const
     {
        return std::strcmp(a, b) < 0;
     }
  };

  class DispatcherEvent : public Event
  {
    public:
      GSAGE_API static const Event::Type FORCE_UNSUBSCRIBE;

      DispatcherEvent(ConstType type) : Event(type) {}
      virtual ~DispatcherEvent() {};
  };

  class EventDispatcher
  {
    public:

      EventDispatcher();
      virtual ~EventDispatcher();
      /**
       * All event bindings
       */
      typedef std::map<Event::ConstType, EventSignalPtr, CmpEventType> EventTypes;

      /**
       * Dispatch event to all subscribers of the type, defined in the event.
       *
       * @param event Abstract event
       *
       * @returns count of handers that processed the response
       */
      int fireEvent(const Event& event);

      /**
       * Remove all listeners from this event dispatcher
       */
      void removeAllListeners();
    private:
      template<class C>
      friend class EventSubscriber;
      /**
       * Check if dispatcher has listeners for event type
       *
       * @param type Event type
       */
      bool hasListenersForType(Event::ConstType type);
      /**
       * Adds event subscriber to the event. Should be called by event subscriber class
       *
       * @param eventType Type of the event
       * @param callback Function binding
       * @param priority Priority of attached event callback
       */
      EventConnection addEventListener(Event::ConstType eventType, EventCallback callback, const int priority = 0);
      EventTypes mSignals;
      std::mutex mMutationMutex;
  };
}

#endif
