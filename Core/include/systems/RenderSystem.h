#ifndef _RenderSystem_H_
#define _RenderSystem_H_

/*
-----------------------------------------------------------------------------
This file is a part of Gsage engine

Copyright (c) 2014-2018 Artem Chernyshev and contributors

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

#include <tuple>
#include <vector>
#include <memory>
#include "Logger.h"
#include "GeometryPrimitives.h"
#include "DataProxy.h"
#include "EventDispatcher.h"
#include "GsageFacade.h"

namespace Gsage {
  class RenderSystem;
  class Geom
  {
    public:
      typedef std::tuple<int*, size_t> Tris;
      typedef std::tuple<float*, size_t> Verts;
      typedef std::tuple<float*, size_t> Normals;

      Geom();
      virtual ~Geom();

      /**
       * Retrieves the vertices stored within this Geom. The verts are an array of floats in which each
       * subsequent three floats are in order the x, y and z coordinates of a vert. The size of this array is
       * always a multiple of three and is exactly 3*getVertCount().
       **/
      Verts getVerts();

      /**
       * Retrieves the tris stored in this Geom.
       * A tri is defined by a sequence of three indexes which refer to an index position in the getVerts() array.
       * Similar to getVerts, the size of this array is a multitude of 3 and is exactly 3*getTriCount().
       **/
      Tris getTris();

      /**
       * Retrieve the normals calculated for this inputGeom. Note that the normals are not exact and are not meant for rendering,
       * but they are good enough for navmesh calculation. Each normal corresponds to one vertex from getVerts() with the same index.
       * The size of the normals array is 3*getVertCount().
       **/
      Normals getNormals();

      /**
       * The axis aligned bounding box minimum of this input Geom.
       **/
      float* getMeshBoundsMin(void);

      /**
       * The axis aligned bounding box maximum of this input Geom.
       **/
      float* getMeshBoundsMax(void);

      /**
       * Check if geom is actually empty
       */
      inline bool empty() { return ntris == 0 && nverts == 0; }

      int* tris;
      size_t ntris;
      float* verts;
      size_t nverts;
      float* normals;
      size_t nnormals;
      float* bmax;
      float* bmin;
  };

  typedef std::unique_ptr<Geom> GeomPtr;

  /**
   * Abstract representation of rendered texture
   */
  class GSAGE_API Texture : public EventDispatcher
  {
    public:
      typedef std::tuple<Gsage::Vector2, Gsage::Vector2, Gsage::Vector2, Gsage::Vector2> UVs;

      static const Event::Type RESIZE;

      static const Event::Type RECREATE;

      static const Event::Type UV_UPDATE;

      static const Event::Type DESTROY;

      Texture(const std::string& name, const DataProxy& params);
      virtual ~Texture();

      /**
       * Update texture data
       *
       * @param buffer buffer to use
       * @param size provided buffer size
       * @param width buffer width
       * @param height buffer height
       */
      virtual void update(const void* buffer, size_t size, int width, int height) = 0;

      /**
       * Update texture data using changed rectangle
       *
       * @param buffer buffer to use
       * @param size provided buffer size
       * @param width buffer width
       * @param height buffer height
       * @param area changed rect
       */
      virtual void update(const void* buffer, size_t size, int width, int height, const Rect<int>& area) = 0;

      /**
       * Set texture size
       *
       * @param width texture width
       * @param height texture height
       */
      virtual void setSize(int width, int height);

      /**
       * Get texture width
       */
      inline Gsage::Vector2 getSrcSize() const { return Gsage::Vector2(mBufferWidth, mBufferHeight); }


      /**
       * Get texture size
       */
      inline std::tuple<int, int> getSize() const { return std::make_tuple(mWidth, mHeight); };

      /**
       * Check if the texture is valid
       *
       * @returns false if render system is not ready for example
       */
      inline bool isValid() const { return mValid; }

      virtual bool hasData() const = 0;

      inline const std::string& getName() const { return mHandle; }

      /**
       * Get UV to use
       */
      Texture::UVs getUVs() const;

      /**
       * Set UV to use
       *
       * @param tl Top left corner
       * @param bl Bottom left corner
       * @param tr Top right corner
       * @param br Bottom right corner
       */
      void setUVs(const Gsage::Vector2& tl, const Gsage::Vector2& bl, const Gsage::Vector2& tr, const Gsage::Vector2& br);

      virtual void updateConfig(const DataProxy& config);

      inline DataProxy& getConfig() { return mParams; }

    protected:

      char* allocateBuffer(size_t size);

      Gsage::Vector2 mUVTL;
      Gsage::Vector2 mUVBL;
      Gsage::Vector2 mUVTR;
      Gsage::Vector2 mUVBR;

      bool mValid;
      int mWidth;
      int mHeight;

      int mBufferWidth;
      int mBufferHeight;

      char* mBuffer;
      size_t mSize;
      DataProxy mParams;

      std::string mHandle;

      std::mutex mLock;

    private:

      void deleteBuffer();
  };

  typedef std::shared_ptr<Texture> TexturePtr;

  // UI Rendering -------------------------------

  // ui render operation vertex type
  struct GSAGE_API UIVertex {
    float x, y;
    float u, v;
    unsigned int diffuse;
  };

  // ui render operation index type
  typedef unsigned short UIIndex;

  /**
   * Abstract UI geometry primitive
   *
   * Represents a single render operation
   * Contains vertex and index buffers
   */
  class GSAGE_API UIGeom {
    public:
      UIGeom()
        : x(0)
        , y(0)
        , scLeft(0)
        , scTop(0)
        , scBottom(INT_MAX)
        , scRight(INT_MAX)
        , texture(nullptr)
      {
      }

      virtual ~UIGeom()
      {
      }

      virtual void updateVertexData(
          const void* vertices,
          const void* indices,
          size_t numVertices,
          size_t numIndices,
          size_t vertexSize = sizeof(UIVertex),
          size_t indexSize = sizeof(UIIndex)
      ) = 0;

      float x;
      float y;
      int scLeft;
      int scTop;
      int scBottom;
      int scRight;
      TexturePtr texture;
  };

  typedef std::shared_ptr<UIGeom> UIGeomPtr;

  /**
   * Abstract representation of texture that is displayed at screen
   */
  class GSAGE_API RenderViewport
  {
    public:
      RenderViewport()
        : mWindow(nullptr)
      {
      }
      // gets render target window
      inline WindowPtr getWindow() {
        return mWindow;
      }

      virtual inline void setWindow(WindowPtr window)
      {
        mWindow = window;
      }
      /**
       * Change viewport resolution
       *
       * @param width pixels
       * @param height pixels
       */
      virtual void setDimensions(int width, int height) = 0;

      /**
       * Set viewport position
       * This is used for mouse event handling
       *
       * @param x
       * @param y
       */
      virtual void setPosition(int x, int y) = 0;

      /**
       * Get viewport resolution
       */
      virtual Vector2 getDimensions() const = 0;

      /**
       * Get viewport position
       */
      virtual Vector2 getPosition() const = 0;

      /**
       * Read current camera view matrix
       * If there is no current camera will return zero matrix
       *
       * @param dest must be float c array of size 16
       */
      virtual void getViewMatrix(float* dest) const = 0;

      /**
       * Read current camera projection matrix
       * If there is no current camera will return zero matrix
       *
       * @param dest must be float c array of size 16
       */
      virtual void getProjectionMatrix(float* dest) const = 0;
    protected:
      WindowPtr mWindow;
  };

  /**
   * Abstract UIRenderer to render geometry
   */
  class GSAGE_API UIRenderer
  {
    public:
      /**
       * UIRenderer configuration
       */
      struct Configuration {
        bool valid;
        SceneBlendType blendType;
        SceneBlendOperation blendOperation;
        SceneBlendOperation blendOperationAlpha;
        SceneBlendFactor srcBlendFactor;
        SceneBlendFactor dstBlendFactor;
        SceneBlendFactor srcBlendFactorAlpha;
        SceneBlendFactor dstBlendFactorAlpha;
        CullingMode cullMode;
        TextureFilterOptions texFiltering;
      };

      static Configuration createConfiguration()
      {
        Configuration res;
        memset(&res, 0, sizeof(Configuration));
        res.valid = true;
        res.blendType = SceneBlendType::SBT_TRANSPARENT_ALPHA;
        res.blendOperation = SceneBlendOperation::SBO_ADD;
        res.blendOperationAlpha = SceneBlendOperation::SBO_ADD;
      }

      UIRenderer();

      /**
       * Set up UIRenderer
       *
       * @param config renderer material options
       * @param render system used for set up
       * @param mgrName UI manager name
       */
      virtual void initialize(Configuration& config, RenderSystem* render, const std::string& mgrName) {
        mRender = render;
      }

      /**
       * Start rendering new frame
       *
       * @param ctxName context name
       */
      virtual void begin(const std::string& ctxName) = 0;

      /**
       * Frame rendering end hook
       */
      virtual void end() {}

      /**
       * Create ui geometry
       * Must be implemented in render system to return geom specific to the render system
       */
      virtual UIGeomPtr createUIGeom() const = 0;

      /**
       * Renders ui geometry
       *
       * @param geom must get concrete render system implementation
       */
      virtual void renderUIGeom(UIGeomPtr geom) = 0;

      /**
       * Create texture
       *
       * @param name texture name
       * @param data texture data
       * @param width texture width
       * @param height texture height
       */
      TexturePtr createTexture(const char* name, void* data, int width, int height, PixelFormat pixelFormat = PixelFormat::PF_R8G8B8A8);

      /**
       * Get render target by ID
       */
      RenderViewport* getViewport(const char* name);
    protected:
      RenderSystem* mRender;
  };

  typedef std::shared_ptr<UIRenderer> UIRendererPtr;

  class GSAGE_API RenderSystem
  {
    public:
      static size_t getPixelSize(PixelFormat format);

      typedef std::string TextureHandle;

      RenderSystem();
      virtual ~RenderSystem();

      /**
       * Get implementation independent Geometry information
       *
       * @param bounds get entities in bounds
       * @param flags filter entities by flags (default is all)
       *
       * @returns GeomPtr
       */
      virtual GeomPtr getGeometry(const BoundingBox& bounds, int flags = 0xFF) = 0;

      /**
       * Get implementation independent Geometry information
       *
       * @param entities filter entities by names
       * @returns GeomPtr
       */
      virtual GeomPtr getGeometry(std::vector<std::string> entities) = 0;

      /**
       * Create texture manually
       *
       * @param handle texture id
       * @param parameters variable parameters that can be used for texture creation
       */
      virtual TexturePtr createTexture(RenderSystem::TextureHandle handle, const DataProxy& parameters) = 0;

      /**
       * Get texture by name
       *
       * @param handle texture id
       */
      virtual TexturePtr getTexture(RenderSystem::TextureHandle handle) = 0;

      /**
       * Delete texture by handle
       *
       * @param handle texture id
       * @returns true if succeed
       */
      virtual bool deleteTexture(RenderSystem::TextureHandle handle) = 0;

      /**
       * Get viewport with name
       *
       * @param name unique viewport ID
       */
      virtual RenderViewport* getViewport(const std::string& name) = 0;

      /**
       * Create UI renderer
       *
       * Must be overridden by render systems to support UI rendering
       * Called by UIManager implementation to set up rendering and then use this renderer to display elements
       */
      virtual UIRendererPtr createUIRenderer() const
      {
        LOG(WARNING) << "Render system does not support UI rendering";
        return nullptr;
      }
  };
}

#endif
