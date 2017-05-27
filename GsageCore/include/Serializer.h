#ifndef _Serializer_H_
#define _Serializer_H_

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

#include <json/json.h>


namespace Gsage {

  struct NotFound {
    // no writer found
  };

  template<class T>
  struct Preprocessor {
    typedef NotFound type;
  };

  template<class T>
  struct Writer {
    typedef NotFound type;
  };

  struct JsonValueWriter {
    bool write(const std::string& key, const int& src, Json::Value& dst)
    {
      dst[key] = src;
      return true;
    }
  };

  template<>
  struct Writer<Json::Value> {
    typedef JsonValueWriter type;
  };

  template<class T>
  class Serializer
  {
    public:
      Serializer() {};
      virtual ~Serializer() {};

      template<class F, class C>
      bool write(const F& src, C& dest)
      {
        return false;
      }

      template<class F, class C>
      bool write(const std::string& key, const T& src, C& dest)
      {
        return false;
      }
  };

  class SerializationFactory
  {
    public:
      template<class T, class C>
      static const T& create(C& src)
      {
        return Serializer<T>::read(src);
      }

      template<class T, class C>
      static bool dump(const T& src, C& dst)
      {
        Serializer<T> s;
        return s.template dump<C>(src, dst);
      }
  };
}

#endif
