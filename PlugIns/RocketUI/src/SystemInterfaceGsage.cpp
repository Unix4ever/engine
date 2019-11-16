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

#include "SystemInterfaceGsage.h"
#include "Logger.h"

namespace Gsage {

  SystemInterfaceGsage::SystemInterfaceGsage()
  {
  }

  SystemInterfaceGsage::~SystemInterfaceGsage()
  {
  }

  // Gets the number of seconds elapsed since the start of the application.
  float SystemInterfaceGsage::GetElapsedTime()
  {
    auto now = std::chrono::steady_clock::now();
    std::chrono::duration<float> elapsedSeconds = now - mPreviousUpdateTime;
    mPreviousUpdateTime = now;
    return elapsedSeconds.count();
  }

  // Logs the specified message.
  bool SystemInterfaceGsage::LogMessage(Rocket::Core::Log::Type type, const Rocket::Core::String& message)
  {
    switch (type)
    {
      case Rocket::Core::Log::LT_ALWAYS:
        LOG(FATAL) << message.CString();
        break;
      case Rocket::Core::Log::LT_ERROR:
      case Rocket::Core::Log::LT_ASSERT:
        LOG(ERROR) << message.CString();
        break;
      case Rocket::Core::Log::LT_WARNING:
        LOG(WARNING) << message.CString();
        break;

      default:
        LOG(INFO) << message.CString();
        break;
    }

    return false;
  }

}
