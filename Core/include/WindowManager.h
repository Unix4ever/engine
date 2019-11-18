#ifndef _WindowManager_H_
#define _WindowManager_H_

/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2017 Gsage Authors

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

#include "DataProxy.h"
#include "EventDispatcher.h"
#include <channel>

namespace Gsage {
  /**
   * Abstract window class
   */
  class Window
  {
    public:
      enum DialogMode {
        FILE_DIALOG_OPEN,
        FILE_DIALOG_OPEN_MULTIPLE,
        FILE_DIALOG_OPEN_FOLDER,
        FILE_DIALOG_SAVE
      };

      enum DialogStatus {
        OKAY,
        CANCEL,
        FAILURE
      };

      enum Cursor {
        None = -1,
        Arrow = 0,
        TextInput,
        ResizeAll,
        ResizeNS,
        ResizeEW,
        ResizeNESW,
        ResizeNWSE,
        Hand
      };

      Window(const std::string& name) : mName(name), mCurrentCursor(Cursor::Arrow) {};
      /**
       * Get window handle
       */
      virtual unsigned long long getWindowHandle() = 0;

      /**
       * Get openGL context
       */
      virtual void* getGLContext() = 0;

      /**
       * Gets window name
       */
      inline const std::string& getName() const { return mName; }

      /**
       * Gets window position
       */
      virtual std::tuple<int, int> getPosition() const = 0;

      /**
       * Sets window position
       *
       * @param x
       * @param y
       */
      virtual void setPosition(int x, int y) = 0;

      /**
       * Get window size
       */
      virtual std::tuple<int, int> getSize() = 0;

      /**
       * Set window size
       *
       * @param width
       * @param height
       */
      virtual void setSize(int width, int height) = 0;

      /**
       * Get display size information
       */
      virtual std::tuple<int, int, int, int> getDisplayBounds() = 0;

      /**
       * Get DPI for the screen where the window is located
       */
      virtual float getScaleFactor() const = 0;

      /**
       * Show window
       */
      virtual void show() = 0;

      /**
       * Hide window
       */
      virtual void hide() = 0;

      /**
       * Change cursor
       *
       * @param c cursor type
       */
      virtual void setCursor(Cursor c) = 0;

      /**
       * Check if window is focused
       */
      virtual bool focused() const = 0;
    protected:
      std::string mName;
      Cursor mCurrentCursor;
  };

  /**
   * Window pointer
   */
  typedef std::shared_ptr<Window> WindowPtr;

  /**
   * Abstract window manager
   */
  class WindowManager : public EventDispatcher
  {
    public:
      typedef std::tuple<std::vector<std::string>, Window::DialogStatus, std::string> DialogResult;
      typedef std::function<void(DialogResult)> DialogCallback;

      WindowManager(const std::string& type);
      virtual ~WindowManager();

      /**
       * Initialize WindowManager
       *
       * @param config Window manager configuration params
       */
      virtual bool initialize(const DataProxy& config) = 0;

      /**
       * Create a window
       *
       * @param create a window
       * @param width window width
       * @param height window height
       * @param fullscreen create a fullscreen window
       * @param params additional parameters
       * @returns newly created window pointer
       */
      virtual WindowPtr createWindow(const std::string& name, unsigned int width, unsigned int height,
          bool fullscreen, const DataProxy& params) = 0;

      /**
       * Close a window
       *
       * @param window window pointer to destroy
       * @returns true if succeed
       */
      virtual bool destroyWindow(WindowPtr window) = 0;

      /**
       * Fire window event
       *
       * @param type event type
       * @param handle handle of the target window
       * @param width current window window
       * @param height current window height
       */
      virtual void fireWindowEvent(Event::ConstType type, unsigned long handle, unsigned int width = 0, unsigned int height = 0);

      /**
       * Get window by name
       *
       * @param name Window name
       *
       * @returns WindowPtr if found
       */
      virtual WindowPtr getWindow(const std::string& name);
      /**
       * Get window by handle
       *
       * @param handle
       *
       * @returns WindowPtr if found
       */
      virtual WindowPtr getWindow(const unsigned long long handle);
      /**
       * Opens file dialog, blocking operation on OSX
       *
       * @param mode Dialog mode
       * @param title Dialog title
       * @param defaultFilePath Default file path
       * @param filters Filters
       *
       * @returns std::tuple: files, status, error
       */
      DialogResult openDialog(
        Window::DialogMode mode,
        const std::string& title,
        const std::string& defaultFilePath,
        const std::vector<std::string> filters
      );

      /**
       * Get window manager type
       */
      inline const std::string& getType() const { return mType; }
    protected:
      const std::string mType;
      std::map<std::string, WindowPtr> mWindows;

      void windowCreated(WindowPtr window);
      bool windowDestroyed(WindowPtr window);

      DialogResult openDialogSync(
        Window::DialogMode mode,
        const std::string& title,
        const std::string& defaultFilePath,
        const std::vector<std::string> filters
      );
  };

  typedef std::shared_ptr<WindowManager> WindowManagerPtr;
}

#endif
