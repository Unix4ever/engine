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

#include "FileLoader.h"
#include <assert.h>

namespace Gsage {
  FileLoader* FileLoader::mInstance = 0;

  FileLoader FileLoader::getSingleton()
  {
    assert(mInstance != 0);
    return *mInstance;
  }

  FileLoader* FileLoader::getSingletonPtr()
  {
    assert(mInstance != 0);
    return mInstance;
  }

  void FileLoader::init(FileLoader::Encoding format, const Dictionary& environment)
  {
    FileLoader::mInstance = new FileLoader(format, environment);
  }

  FileLoader::FileLoader(FileLoader::Encoding format, const Dictionary& environment)
    : mFormat(format)
    , mEnvironment(environment)
  {
  }

  FileLoader::~FileLoader()
  {
  }

  bool FileLoader::load(const std::string& path, const Dictionary& params, Dictionary& dest) const
  {
    Dictionary p = getUnionDict(mEnvironment, params);
    auto pair = loadFile(path);
    if (!pair.second) {
      return false;
    }

    if(!parse(pair.first, dest))
      return false;

    unionDict(dest, p);
    return true;
  }

  std::pair<Dictionary, bool> FileLoader::load(const std::string& path, const Dictionary& params) const
  {
    Dictionary res;
    bool success = load(path, params, res);
    return std::make_pair(res, success);
  }

  std::pair<Dictionary, bool> FileLoader::load(const std::string& path) const
  {
    return load(path, Dictionary());
  }

  void FileLoader::dump(const std::string& path, const Dictionary& value) const
  {
    std::ofstream stream(path);
    switch(mFormat) {
      case Json:
        dumpJson(stream, value);
        break;
      case Msgpack:
        dumpMsgPack(stream, value);
        break;
    }
    stream.close();
  }

  std::pair<std::string, bool> FileLoader::loadFile(const std::string& path) const
  {
    std::ifstream stream(path);
    std::string res;
    bool success = true;
    if(!stream) {
      return std::make_pair("", false);
    }
    try
    {
      stream.seekg(0, std::ios::end);
      res.reserve(stream.tellg());
      stream.seekg(0, std::ios::beg);
      res.assign((std::istreambuf_iterator<char>(stream)),
                  std::istreambuf_iterator<char>());
    }
    catch(std::exception& e)
    {
      success = false;
      LOG(ERROR) << "Failed to read file: " << path << ", reason: " << e.what();
    }

    stream.close();

    return std::make_pair(res, success);
  }

  bool FileLoader::parse(const std::string& data, Dictionary& dest) const
  {
    switch(mFormat) {
      case Json:
        return parseJson(data, dest);
      case Msgpack:
        return parseMsgPack(data, dest);
    }
    return false;
  }

}
