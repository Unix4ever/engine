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

#include "KeyboardEvent.h"

namespace Gsage {
  const Event::Type KeyboardEvent::KEY_DOWN = "KeyboardEvent::KEY_DOWN";
  const Event::Type KeyboardEvent::KEY_UP = "KeyboardEvent::KEY_UP";

  KeyboardEvent::KeyboardEvent(Event::ConstType type, const Key& code, const unsigned int t, const unsigned int modState)
    : Event(type)
    , key(code)
    , text(t)
    , modifierState(modState)
  {
  }

  KeyboardEvent::~KeyboardEvent()
  {
  }

  bool KeyboardEvent::isModifierDown(const Modifier& modifier) const
  {
    return (modifierState & modifier) != 0;
  }

  unsigned int KeyboardEvent::getModifiersState() const
  {
    return modifierState;
  }

  const Event::Type TextInputEvent::INPUT = "TextInputEvent::INPUT";

  TextInputEvent::TextInputEvent(Event::ConstType type, const char* text)
    : Event(type)
    , mUTF8Chars(text)
  {
  }

  TextInputEvent::~TextInputEvent()
  {
  }

  const char* TextInputEvent::getText() const
  {
    return mUTF8Chars;
  }
}
