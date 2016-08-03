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

#include "EngineSystem.h"

using namespace Gsage;

EngineSystem::EngineSystem() :
  mReady(false),
  mConfigDirty(false),
  mEnabled(true)
{
}

EngineSystem::~EngineSystem()
{
}

bool EngineSystem::configure(const DataNode& config)
{
  mConfig = config;
  mEnabled = mConfig.get("enabled", true);
  mConfigDirty = true;
  return true;
}

const DataNode& EngineSystem::getConfig()
{
  return mConfig;
}

bool EngineSystem::initialize(const DataNode& settings)
{
  return mReady = true;
}

void EngineSystem::setEngineInstance(Engine* value)
{
  mEngine = value;
}

void EngineSystem::resetEngineInstance()
{
  mEngine = NULL;
}

void EngineSystem::configUpdated()
{
  mConfigDirty = false;
}

void EngineSystem::setEnabled(bool value)
{
  mEnabled = value;
  mConfig.put("enabled", value);
}
