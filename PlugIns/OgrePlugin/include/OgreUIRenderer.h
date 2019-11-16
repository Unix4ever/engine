#ifndef _UIRenderer_H_
#define _UIRenderer_H_

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

#include "EventSubscriber.h"
#include "OgreGeom.h"
#include "RenderTarget.h"
#include "Definitions.h"

#if OGRE_VERSION >= 0x020100
#include <OgrePsoCacheHelper.h>
#endif

namespace Gsage {
  class OgreRenderSystem;

    /**
   * UI geometry for V1 rendering
   */
  class OgreUIGeom : public UIGeom, public Ogre::Renderable
  {
    public:
      OgreUIGeom(Ogre::uint32 hash, const Ogre::String& materialName);
      ~OgreUIGeom();

      Ogre::uint32 hash;

      /**
       * Update vertex and index buffer
       */
      void updateVertexData(
          const void* vertices,
          const void* indices,
          size_t numVertices,
          size_t numIndices,
          size_t vertexSize = sizeof(UIVertex),
          size_t indexSize = sizeof(UIIndex)
      );

      Ogre::Real getSquaredViewDepth(const Ogre::Camera* cam) const {
        (void)cam;
        return 0;
      }

      void setMaterial(const Ogre::String& matName);
      void setMaterial(const Ogre::MaterialPtr & material);
      const Ogre::MaterialPtr& getMaterial(void) const;
      void getWorldTransforms(Ogre::Matrix4* xform) const;
      void getRenderOperation(OgreV1::RenderOperation& op
#if OGRE_VERSION >= 0x020100
        , bool casterPass
#endif
      );
      const Ogre::LightList& getLights(void) const;
#if OGRE_VERSION >= 0x020100
      Ogre::VertexElement2VecVec mVertexElement2VecVec;
#endif
    private:
      int mVertexBufferSize;
      int mIndexBufferSize;
      OgreV1::VertexData mVertexData;
      OgreV1::IndexData mIndexData;
      OgreV1::RenderOperation mRenderOp;
  };

  class OgreUIRenderer : public UIRenderer
  {
    public:
      OgreUIRenderer();
      ~OgreUIRenderer();

      /**
       * Resets renderer before new frame
       */
      void begin(const std::string& ctxName);

      /**
       * Create UI geometry
       */
      UIGeomPtr createUIGeom() const;

      /**
       * Render UI geometry
       *
       * @param geom UI geometry primitive
       */
      void renderUIGeom(UIGeomPtr geom);

      /**
       * Resets render after frame ends
       */
      void end();

      /**
       * Initializes OgreUIRenderer
       */
      void initialize(UIRenderer::Configuration& config, RenderSystem* render, const std::string& mgrName);
    private:
      void setFiltering(Ogre::TextureFilterOptions mode);
      Ogre::Pass*	mPass;
      Ogre::TextureUnitState* mTexUnit;
#if OGRE_VERSION >= 0x020100
      Ogre::PsoCacheHelper* mPsoCache;
      std::vector<Ogre::uint32> mHashes;
#endif
      Ogre::SceneManager* mSceneMgr;
      RenderTargetPtr mRenderTarget;
      Ogre::String mMaterialName;
  };
}

#endif
