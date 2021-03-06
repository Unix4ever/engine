/*
   -----------------------------------------------------------------------------
   This source file is part of OGRE
   (Object-oriented Graphics Rendering Engine)
   For the latest info, see http://www.ogre3d.org/

   Copyright (c) 2000-2014 Torus Knot Software Ltd

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
#include "OgreStableHeaders.h"

#include "ogre/ImguiRenderable.h"

#include "OgreHardwareBufferManager.h"
#include "OgreMaterialManager.h"
#include <OgreSceneManager.h>
#include <OgreHardwareVertexBuffer.h>
#include <OgreHardwareIndexBuffer.h>
#include <OgreHardwarePixelBuffer.h>


namespace Gsage
{
  ImGUIRenderable::ImGUIRenderable():
    mVertexBufferSize(5000)
    ,mIndexBufferSize(10000)
  {
    initImGUIRenderable();
    //By default we want ImGUIRenderables to still work in wireframe mode
    setPolygonModeOverrideable( false );
  }

  //-----------------------------------------------------------------------------------
  void ImGUIRenderable::initImGUIRenderable(void)
  {
    // use identity projection and view matrices
    mUseIdentityProjection  = true;
    mUseIdentityView        = true;

    mRenderOp.vertexData = OGRE_NEW Ogre::VertexData();
    mRenderOp.indexData  = OGRE_NEW Ogre::IndexData();

    mRenderOp.vertexData->vertexCount = 0;
    mRenderOp.vertexData->vertexStart = 0;

    mRenderOp.indexData->indexCount = 0;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.operationType             = Ogre::RenderOperation::OT_TRIANGLE_LIST;
    mRenderOp.useIndexes                                    = true;
    mRenderOp.useGlobalInstancingVertexBufferIsAvailable    = false;

    Ogre::VertexDeclaration* decl     = mRenderOp.vertexData->vertexDeclaration;

    // vertex declaration
    size_t offset = 0;
    decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_POSITION);
    offset += Ogre::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);
    offset += Ogre::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);

    // set basic white material
    this->setMaterial("imgui/material");
  }

  //-----------------------------------------------------------------------------------

  ImGUIRenderable::~ImGUIRenderable()
  {
    OGRE_DELETE mRenderOp.vertexData;
  }

  //-----------------------------------------------------------------------------------

  void ImGUIRenderable::setMaterial( const Ogre::String& matName )
  {
    mMaterial = Ogre::MaterialManager::getSingleton().getByName( matName );
    if( mMaterial.isNull() )
    {
      OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not find material " + matName,
          "ImGUIRenderable::setMaterial");
    }

    // Won't load twice anyway
    mMaterial->load();
  }

  //-----------------------------------------------------------------------------------

  void ImGUIRenderable::setMaterial(const Ogre::MaterialPtr & material)
  {
    mMaterial = material;
  }

  //-----------------------------------------------------------------------------------

  const Ogre::MaterialPtr& ImGUIRenderable::getMaterial(void) const
  {
    return mMaterial;
  }

  //-----------------------------------------------------------------------------------

  void ImGUIRenderable::updateVertexData(ImDrawData* draw_data, unsigned int cmdIndex)
  {
    Ogre::VertexBufferBinding* bind   = mRenderOp.vertexData->vertexBufferBinding;

    const ImDrawList* cmd_list = draw_data->CmdLists[cmdIndex];

    if (bind->getBindings().empty() || mVertexBufferSize != cmd_list->VtxBuffer.size())
    {
      mVertexBufferSize = cmd_list->VtxBuffer.size();

      bind->setBinding(0, Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(sizeof(ImDrawVert), mVertexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY));

    }
    if (mRenderOp.indexData->indexBuffer.isNull() || mIndexBufferSize != cmd_list->IdxBuffer.size())
    {
      mIndexBufferSize = cmd_list->IdxBuffer.size();

      mRenderOp.indexData->indexBuffer=
        Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, mIndexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY);
    }

    // Copy all vertices
    ImDrawVert* vtx_dst = (ImDrawVert*)(bind->getBuffer(0)->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    ImDrawIdx* idx_dst = (ImDrawIdx*)(mRenderOp.indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));

    memcpy(vtx_dst, &cmd_list->VtxBuffer[0], mVertexBufferSize * sizeof(ImDrawVert));
    memcpy(idx_dst, &cmd_list->IdxBuffer[0], mIndexBufferSize * sizeof(ImDrawIdx));

    mRenderOp.vertexData->vertexStart = 0;
    mRenderOp.vertexData->vertexCount =  cmd_list->VtxBuffer.size();
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.indexData->indexCount =  cmd_list->IdxBuffer.size();

    bind->getBuffer(0)->unlock();
    mRenderOp.indexData->indexBuffer->unlock();
  }

  void ImGUIRenderable::updateVertexData(const ImDrawVert* vtxBuf, const ImDrawIdx* idxBuf, unsigned int vtxCount, unsigned int idxCount)
  {
    Ogre::VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

    if (bind->getBindings().empty() || mVertexBufferSize != vtxCount)
    {
      mVertexBufferSize = vtxCount;

      bind->setBinding(0, Ogre::HardwareBufferManager::getSingleton().createVertexBuffer(sizeof(ImDrawVert), mVertexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY));
    }
    if (mRenderOp.indexData->indexBuffer.isNull() || mIndexBufferSize != idxCount)
    {
      mIndexBufferSize = idxCount;

      mRenderOp.indexData->indexBuffer =
        Ogre::HardwareBufferManager::getSingleton().createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, mIndexBufferSize, Ogre::HardwareBuffer::HBU_WRITE_ONLY);
    }

    // Copy all vertices
    ImDrawVert* vtxDst = (ImDrawVert*)(bind->getBuffer(0)->lock(Ogre::HardwareBuffer::HBL_DISCARD));
    ImDrawIdx* idxDst = (ImDrawIdx*)(mRenderOp.indexData->indexBuffer->lock(Ogre::HardwareBuffer::HBL_DISCARD));

    memcpy(vtxDst, vtxBuf, mVertexBufferSize * sizeof(ImDrawVert));
    memcpy(idxDst, idxBuf, mIndexBufferSize * sizeof(ImDrawIdx));

    mRenderOp.vertexData->vertexStart = 0;
    mRenderOp.vertexData->vertexCount = vtxCount;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.indexData->indexCount = idxCount;

    bind->getBuffer(0)->unlock();
    mRenderOp.indexData->indexBuffer->unlock();
  }

  //-----------------------------------------------------------------------------------

  void ImGUIRenderable::getWorldTransforms( Ogre::Matrix4* xform ) const
  {
    *xform = Ogre::Matrix4::IDENTITY;
  }

  //-----------------------------------------------------------------------------------

  void ImGUIRenderable::getRenderOperation(Ogre::RenderOperation& op)
  {
    op = mRenderOp;
  }

  //-----------------------------------------------------------------------------------

  const Ogre::LightList& ImGUIRenderable::getLights(void) const
  {
    static const Ogre::LightList l;
    return l;
  }
}
