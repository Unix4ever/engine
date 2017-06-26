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

#ifndef _OgreRenderSystem_H_
#define _OgreRenderSystem_H_

#include <OgreRoot.h>
#include <OgreRenderWindow.h>
#include <OgreEntity.h>
#include <OgreConfigFile.h>
#include <OgreSceneManager.h>
#include <OgreRenderSystem.h>
#include <OgreWindowEventUtilities.h>
#include <OgreStringConverter.h>
#include <OgreLog.h>
#include <OgreRenderQueueListener.h>

#include "ComponentStorage.h"
#include "components/RenderComponent.h"
#include "ogre/OgreObjectManager.h"
#include "EventDispatcher.h"
#include "OgreInteractionManager.h"

#include "RenderSystem.h"

static const std::string OGRE_SECTION        = "OgreRenderer";
static const std::string OGRE_PLUGINS_PATH   = "PluginsPath";
static const std::string OGRE_CONFIG_PATH    = "ConfigPath";
static const std::string OGRE_RESOURCES      = "Resources";


namespace Ogre
{
  class FontManager;
  class ManualMovableTextRendererFactory;
  class Viewport;
  class RenderWindow;
  class SceneManager;
}

namespace Gsage
{
  class AnimationScheduler;
  class CameraFactory;
  class CameraController;
  class ResourceManager;
  class Entity;
  class Engine;
  class EngineEvent;
  class OgreInteractionManager;
  class WindowEventListener;

  /**
   * Class, used to redirect all ogre output to the easylogging++
   */
  class OgreLogRedirect : public Ogre::LogListener
  {
     void messageLogged( const std::string& message,
              Ogre::LogMessageLevel lml,
              bool maskDebug,
              const std::string& system,
              bool& skipMessage
              );
  };

  class OgreRenderSystem : public ComponentStorage<RenderComponent>, public Ogre::RenderQueueListener, public EventDispatcher, public RenderSystem
  {
    public:
      static const std::string ID;
      static const std::string CAMERA_CHANGED;

      OgreRenderSystem();
      virtual ~OgreRenderSystem();

      // --------------------------------------------------------------------------------
      // Gsage::ComponentStorage implementation
      // --------------------------------------------------------------------------------

      /**
       * Intializes ogre render system
       * @param settings DataProxy with initial settings for the render system
       */
      virtual bool initialize(const DataProxy& settings);

      /**
       * Initializes RenderComponent: creates ogre node, lights and etc.
       *
       * @param component RenderComponent component to initialize
       * @param data DataProxy with node settings
       * @returns false if failed to initialize component for some reason
       */
      virtual bool fillComponentData(RenderComponent* component, const DataProxy& data);

      /**
       * Update render system
       * @param time Elapsed time
       */
      virtual void update(const double& time);

      /**
       * Update RenderComponent
       *
       * @param component RenderComponent to update
       * @param entity Entity that owns that component
       * @param time Elapsed time
       */
      virtual void updateComponent(RenderComponent* component, Entity* entity, const double& time);

      /**
       * Remove render component from the system. Removes all related visual nodes
       *
       * @param component RenderComponent component to remove
       */
      virtual bool removeComponent(RenderComponent* component);

      /**
       * Reconfigure render system
       *
       * @param config DataProxy with configs
       */
      virtual bool configure(const DataProxy& config);

      /**
       * Get current configs of the system
       */
      DataProxy& getConfig();

      // --------------------------------------------------------------------------------
      // Ogre::RenderQueueListener implementation
      // --------------------------------------------------------------------------------

      /**
       * Callback for ui updating
       */
      virtual void renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& skipThisInvocation);

      // --------------------------------------------------------------------------------
      /**
       * Register new type of factory in the render system
       */
      template<typename T>
      void registerElement() { mObjectManager.registerElement<T>(); }

      /**
       * Unregister factory type
       */
      template<typename T>
      bool unregisterElement() { return mObjectManager.unregisterElement<T>(); }

      /**
       * Get ogre render window
       */
      Ogre::RenderWindow* getRenderWindow() { return mWindow; }

      /**
       * Get ogre scene manager
       */
      Ogre::SceneManager* getSceneManager() {return mSceneManager; }

      /**
       * Get ogre render system
       */
      Ogre::RenderSystem* getRenderSystem() {return mRenderSystem;}

      typedef std::vector<Entity*> Entities;
      /**
       * Enumerable of Ogre::Entity
       */
      typedef std::vector<Ogre::Entity*> OgreEntities;
      /**
       * Gets all scene entities
       *
       * @param query Filter entities by some query, default is all
       */
      OgreEntities getEntities(const unsigned int& query = 0xFF);
      /**
       * Get currently active camera controller
       */
      CameraController* getCamera();
      /**
       * Get currently active ogre camera
       */
      Ogre::Camera* getOgreCamera();
      /**
       * Get objects in radius
       * @param position Point to search around
       * @param distance Radius of the sphere query
       * @returns list of entities
       */
      Entities getObjectsInRadius(const Ogre::Vector3& center, const float& distance, const unsigned int flags = 0xFF, const std::string& id = "");

      /**
       * Get main viewport
       */
      Ogre::Viewport* getViewport();

      virtual unsigned int getFBOID(const std::string& target = "") const;

      /**
       * Set render target width
       *
       * @param target Render target name
       */
      virtual void setWidth(unsigned int width, const std::string& target = "");

      /**
       * Get render window width
       */
      virtual unsigned int getWidth(const std::string& target = "") const;

      /**
       * Set render target height
       *
       * @param target Render target name
       */
      virtual void setHeight(unsigned int height, const std::string& target = "");

      /**
       * Get render window height
       */
      virtual unsigned int getHeight(const std::string& target = "") const;

      /**
       * Set size of render target
       *
       * @param target Render target name
       */
      virtual void setSize(unsigned int width, unsigned int height, const std::string& target = "");

      /**
       * Render camera to texture with name
       *
       * @param cameraName Name of the camera to use
       * @param target Name of RTT to render to
       */
      void renderCameraToTarget(const std::string& cameraName, const std::string& target);
    protected:
      /**
       * Update current camera
       * @param camera New camera instance
       */
      void updateCurrentCamera(Ogre::Camera* camera);

      void createRttTexture(const std::string& name, unsigned int width,
          unsigned int height, unsigned int samples, Ogre::PixelFormat pf);

      Ogre::Root* mRoot;
      Ogre::SceneManager* mSceneManager;
      Ogre::RenderSystem* mRenderSystem;
      Ogre::RenderWindow* mWindow;
      Ogre::Camera* mCurrentCamera;
      Ogre::LogManager* mLogManager;
      Ogre::FontManager* mFontManager;
      Ogre::ManualMovableTextRendererFactory* mManualMovableTextParticleFactory;
      Ogre::Viewport* mViewport;

      CameraController* mCameraController;

      OgreLogRedirect mLogRedirect;
      CameraFactory* mCameraFactory;
      OgreObjectManager mObjectManager;
      OgreInteractionManager* mOgreInteractionManager;
      WindowEventListener* mWindowEventListener;

      ResourceManager* mResourceManager;

      typedef std::queue<DataProxy> ComponentLoadQueue;
      ComponentLoadQueue mLoadQueue;

      typedef std::map<std::string, Ogre::TexturePtr> RttTextures;
      RttTextures mRttTextures;
  };
}

#endif
