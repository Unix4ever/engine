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

#include "RenderInterfaceGsage.h"
#include "GsageDefinitions.h"

#define GSAGE_COL32_R_SHIFT    0
#define GSAGE_COL32_G_SHIFT    8
#define GSAGE_COL32_B_SHIFT    16
#define GSAGE_COL32_A_SHIFT    24
#define GSAGE_COL32_A_MASK     0xFF000000
#define GSAGE_COL32(R,G,B,A)    (((unsigned int)(A)<<GSAGE_COL32_A_SHIFT) | ((unsigned int)(B)<<GSAGE_COL32_B_SHIFT) | ((unsigned int)(G)<<GSAGE_COL32_G_SHIFT) | ((unsigned int)(R)<<GSAGE_COL32_R_SHIFT))


namespace Gsage {

  // The structure created for each texture loaded by Rocket for Ogre.
  struct RocketGsageTexture
  {
    RocketGsageTexture(TexturePtr texture) : texture(texture)
    {
    }

    TexturePtr texture;
  };

  // The structure created for each set of geometry that Rocket compiles. It stores the vertex and index buffers and the
  // texture associated with the geometry, if one was specified.
  struct RocketGsageGeomHandle
  {
    RocketGsageGeomHandle(UIGeomPtr g)
      : geom(g)
    {
    }

    // object that represents actual geometry data
    UIGeomPtr geom;
  };

  RenderInterfaceGsage::RenderInterfaceGsage(RenderSystem* render)
    : mRender(render)
    , mIntermediateVtxBuffer(NULL)
    , mIntermediateVtxBufferSize(0)
    , mIntermediateIdxBuffer(NULL)
    , mIntermediateIdxBufferSize(0)
    , mCurrentDrawList(0)
  {
    /*
    // Configure the colour blending mode.
    colour_blend_mode.blendType = Ogre::LBT_COLOUR;
    colour_blend_mode.source1 = Ogre::LBS_DIFFUSE;
    colour_blend_mode.source2 = Ogre::LBS_TEXTURE;
    colour_blend_mode.operation = Ogre::LBX_MODULATE;

    // Configure the alpha blending mode.
    alpha_blend_mode.blendType = Ogre::LBT_ALPHA;
    alpha_blend_mode.source1 = Ogre::LBS_DIFFUSE;
    alpha_blend_mode.source2 = Ogre::LBS_TEXTURE;
    alpha_blend_mode.operation = Ogre::LBX_MODULATE;
    */

    /*mScissorEnable = false;

    mScissorLeft = 0;
    mScissorTop = 0;
    mScissorRight = (int) ;
    mScissorBottom = (int) window_height;*/
  }

  RenderInterfaceGsage::~RenderInterfaceGsage()
  {
    if(mIntermediateVtxBuffer) {
      delete mIntermediateVtxBuffer;
    }
    if(mIntermediateIdxBuffer) {
      delete mIntermediateIdxBuffer;
    }
  }

  // Called by Rocket when it wants to render geometry that it does not wish to optimise.
  void RenderInterfaceGsage::RenderGeometry(Rocket::Core::Vertex* ROCKET_UNUSED_PARAMETER(vertices), int ROCKET_UNUSED_PARAMETER(numVertices), int* ROCKET_UNUSED_PARAMETER(indices), int ROCKET_UNUSED_PARAMETER(numIndices), Rocket::Core::TextureHandle ROCKET_UNUSED_PARAMETER(texture), const Rocket::Core::Vector2f& ROCKET_UNUSED_PARAMETER(translation))
  {
    ROCKET_UNUSED(vertices);
    ROCKET_UNUSED(numVertices);
    ROCKET_UNUSED(indices);
    ROCKET_UNUSED(numIndices);
    ROCKET_UNUSED(texture);
    ROCKET_UNUSED(translation);
  }

  // Called by Rocket when it wants to compile geometry it believes will be static for the forseeable future.
  Rocket::Core::CompiledGeometryHandle RenderInterfaceGsage::CompileGeometry(Rocket::Core::Vertex* vertices, int numVertices, int* indices, int numIndices, Rocket::Core::TextureHandle handle)
  {
    UIGeomPtr geometry = mRender->createUIGeom();
    if(!geometry) {
      return 0;
    }

    if(numVertices > mIntermediateVtxBufferSize) {
      if(mIntermediateVtxBuffer) {
        delete mIntermediateVtxBuffer;
      }

      mIntermediateVtxBuffer = new UIVertex[numVertices]();
      if(!mIntermediateVtxBuffer) {
        return 0;
      }
      mIntermediateVtxBufferSize = numVertices;
    }

    if(numIndices > mIntermediateIdxBufferSize) {

      if(mIntermediateIdxBuffer) {
        delete mIntermediateIdxBuffer;
      }

      mIntermediateIdxBuffer = new UIIndex[numIndices]();
      if(!mIntermediateIdxBuffer) {
        return 0;
      }
      mIntermediateIdxBufferSize = numIndices;
    }

    for (int i = 0; i < numVertices; ++i)
    {
      mIntermediateVtxBuffer[i].x = vertices[i].position.x;
      mIntermediateVtxBuffer[i].y = vertices[i].position.y;
      mIntermediateVtxBuffer[i].z = 0;

      mIntermediateVtxBuffer[i].u = vertices[i].tex_coord[0];
      mIntermediateVtxBuffer[i].v = vertices[i].tex_coord[1];

      mIntermediateVtxBuffer[i].diffuse = GSAGE_COL32(
          vertices[i].colour.red / 255.0f,
          vertices[i].colour.green / 255.0f,
          vertices[i].colour.blue / 255.0f,
          vertices[i].colour.alpha / 255.0f
      );
    }

    for (int i = 0; i < numIndices; ++i) {
      mIntermediateIdxBuffer[i] = (UIIndex)indices[i];
    }

    geometry->texture = handle == 0 ? 0 : ((RocketGsageTexture*)handle)->texture;
    geometry->updateVertexData(
        mIntermediateVtxBuffer,
        numVertices,
        mIntermediateIdxBuffer,
        numIndices
    );

    //DrawCmd& cmd = mDrawList.append();
    /*DrawCmd* geometry = mRender->createDrawCmd(DrawCmd::OT_TRIANGLE_LIST, numVertices, numIndices, texture);

    geometry->texture = texture == 0 ? 0 : (RocketGsageTexture*) texture;

    geometry->vertexData = mRender->allocateVertexData();
    geometry->vertexData->vertexStart = 0;
    geometry->vertexData->vertexCount = numVertices;

    geometry->indexData = mRender->allocateIndexData();
    geometry->indexData->indexStart = 0;
    geometry->indexData->indexCount = numIndices;

    geometry->operationType = DrawCmd::OT_TRIANGLE_LIST;

    // Set up the vertex declaration.
    VertexDeclaration* vertexDeclaration = geometry->vertexData->vertexDeclaration;
    size_t element_offset = 0;
    vertexDeclaration->addElement(0, element_offset, DrawCmd::VET_FLOAT3, DrawCmd::VES_POSITION);
    element_offset += Ogre::VertexElement::getTypeSize(DrawCmd::VET_FLOAT3);
    vertexDeclaration->addElement(0, element_offset, DrawCmd::VET_COLOUR, DrawCmd::VES_DIFFUSE);
    element_offset += Ogre::VertexElement::getTypeSize(DrawCmd::VET_COLOUR);
    vertexDeclaration->addElement(0, element_offset, DrawCmd::VET_FLOAT2, DrawCmd::VES_TEXTURE_COORDINATES);


    // Create the vertex buffer.
    Ogre::HardwareVertexBufferSharedPtr vertex_buffer = Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(vertexDeclaration->getVertexSize(0), numVertices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    geometry->render_operation.vertexData->vertexBufferBinding->setBinding(0, vertex_buffer);

    // Fill the vertex buffer.
    RocketGsageVertex* ogre_vertices = (RocketGsageVertex*) vertex_buffer->lock(0, vertex_buffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_NORMAL);
    for (int i = 0; i < numVertices; ++i)
    {
      ogre_vertices[i].x = vertices[i].position.x;
      ogre_vertices[i].y = vertices[i].position.y;
      ogre_vertices[i].z = 0;

      Ogre::ColourValue diffuse(vertices[i].colour.red / 255.0f, vertices[i].colour.green / 255.0f, vertices[i].colour.blue / 255.0f, vertices[i].colour.alpha / 255.0f);
      render_system->convertColourValue(diffuse, &ogre_vertices[i].diffuse);

      ogre_vertices[i].u = vertices[i].tex_coord[0];
      ogre_vertices[i].v = vertices[i].tex_coord[1];
    }
    vertex_buffer->unlock();


    // Create the index buffer.
    Ogre::HardwareIndexBufferSharedPtr index_buffer = Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_32BIT, numIndices, Ogre::HardwareBuffer::HBU_STATIC_WRITE_ONLY);
    geometry->render_operation.indexData->indexBuffer = index_buffer;
    geometry->render_operation.useIndexes = true;

    // Fill the index buffer.
    void* ogre_indices = index_buffer->lock(0, index_buffer->getSizeInBytes(), Ogre::HardwareBuffer::HBL_NORMAL);
    memcpy(ogre_indices, indices, sizeof(unsigned int) * numIndices);
    index_buffer->unlock();*/

    return reinterpret_cast<Rocket::Core::CompiledGeometryHandle>(new RocketGsageGeomHandle(geometry));
  }

  // Called by Rocket when it wants to render application-compiled geometry.
  void RenderInterfaceGsage::RenderCompiledGeometry(Rocket::Core::CompiledGeometryHandle h, const Rocket::Core::Vector2f& translation)
  {
    RocketGsageGeomHandle* handle = (RocketGsageGeomHandle*)h;
    handle->geom->x = translation.x;
    handle->geom->y = translation.y;
    mCurrentDrawList->append(handle->geom);

    /*Ogre::Root* root = Ogre::Root::getSingletonPtr();
    if(!root)
      return;
    render_system = root->getRenderSystem();

    Ogre::Matrix4 transform;
    transform.makeTrans(translation.x, translation.y, 0);
    render_system->_setWorldMatrix(transform);

    RocketGsageCompiledGeometry* ogre3d_geometry = (RocketGsageCompiledGeometry*) geometry;

    if (ogre3d_geometry->texture != NULL)
    {
      render_system->_setTexture(0, true, ogre3d_geometry->texture->texture);

      // Ogre can change the blending modes when textures are disabled - so in case the last render had no texture,
      // we need to re-specify them.
      render_system->_setTextureBlendMode(0, colour_blend_mode);
      render_system->_setTextureBlendMode(0, alpha_blend_mode);
    }
    else
      render_system->_disableTextureUnit(0);

    render_system->_render(ogre3d_geometry->render_operation);*/
  }

  // Called by Rocket when it wants to release application-compiled geometry.
  void RenderInterfaceGsage::ReleaseCompiledGeometry(Rocket::Core::CompiledGeometryHandle h)
  {
    RocketGsageGeomHandle* handle = (RocketGsageGeomHandle*)h;
    delete handle;
  }

  // Called by Rocket when it wants to enable or disable scissoring to clip content.
  void RenderInterfaceGsage::EnableScissorRegion(bool enable)
  {
    //mScissorEnable = enable;

    /*if (!mScissorEnable)
      render_system->setScissorTest(false);
    else
      render_system->setScissorTest(true, mScissorLeft, mScissorTop, mScissorRight, mScissorBottom);*/
  }

  // Called by Rocket when it wants to change the scissor region.
  void RenderInterfaceGsage::SetScissorRegion(int x, int y, int width, int height)
  {
    //if (mScissorEnable)
    //  render_system->setScissorTest(true, mScissorLeft, mScissorTop, mScissorRight, mScissorBottom);
  }

  // Called by Rocket when a texture is required by the library.
  bool RenderInterfaceGsage::LoadTexture(Rocket::Core::TextureHandle& textureHandle, Rocket::Core::Vector2i& textureDimensions, const Rocket::Core::String& source)
  {
    RenderSystem::TextureHandle s(source.CString());
    if(mRender->getTexture(s)) {
      mRender->deleteTexture(s);
    }

    DataProxy params;
    params.put("path", s);
    params.put("scalemode", "normal");
    params.put("width", 1);
    params.put("height", 1);
    TexturePtr texture = mRender->createTexture(s, params);

    if (!texture)
      return false;

    std::tie(textureDimensions.x, textureDimensions.y) = texture->getSize();
    textureHandle = reinterpret_cast<Rocket::Core::TextureHandle>(new RocketGsageTexture(texture));
    return true;
  }

  // Called by Rocket when a texture is required to be built from an internally-generated sequence of pixels.
  bool RenderInterfaceGsage::GenerateTexture(Rocket::Core::TextureHandle& textureHandle, const Rocket::Core::byte* source, const Rocket::Core::Vector2i& sourceDimensions)
  {
    static int textureID = 1;
    const char* textureName = Rocket::Core::String(16, "%d", textureID++).CString();

    DataProxy params;
    params.put("width", sourceDimensions.x);
    params.put("height", sourceDimensions.y);
    params.put("pixelFormat", (int)PixelFormat::PF_A8B8G8R8);
    TexturePtr texture = mRender->createTexture(textureName, params);
    if(!texture) {
      return false;
    }

    size_t size = sourceDimensions.x * sourceDimensions.y * sizeof(unsigned int);
    texture->update(source, size, sourceDimensions.x, sourceDimensions.y);
    textureHandle = reinterpret_cast<Rocket::Core::TextureHandle>(new RocketGsageTexture(texture));
    return true;
  }

  // Called by Rocket when a loaded texture is no longer required.
  void RenderInterfaceGsage::ReleaseTexture(Rocket::Core::TextureHandle h)
  {
    RocketGsageTexture* handle = ((RocketGsageTexture*) h);
    mRender->deleteTexture(handle->texture->getName());
    delete handle;
  }

  // Returns the native horizontal texel offset for the renderer.
  float RenderInterfaceGsage::GetHorizontalTexelOffset()
  {
    //return -mRender->getHorizontalTexelOffset();
    return 0.0f;
  }

  // Returns the native vertical texel offset for the renderer.
  float RenderInterfaceGsage::GetVerticalTexelOffset()
  {
    //return -mRender->getVerticalTexelOffset();
    return 0.0f;
  }
}
