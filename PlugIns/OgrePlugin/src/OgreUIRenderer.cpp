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

#include "OgreUIRenderer.h"
#include "UIShaders.h"
#include "systems/OgreRenderSystem.h"
#include "ManualTextureManager.h"

#include <OgreMaterialManager.h>
#include <OgreTexture.h>
#include <OgreTextureManager.h>
#include <OgreViewport.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTarget.h>

#include "Engine.h"

#if OGRE_VERSION >= 0x020100
#include <RenderSystems/GL3Plus/OgreGL3PlusRenderSystem.h>
#include <CommandBuffer/OgreCbDrawCall.h>
#include <CommandBuffer/OgreCbPipelineStateObject.h>
#include <CommandBuffer/OgreCommandBuffer.h>
#include <OgreHlmsPbsPrerequisites.h>
#include <OgreHlms.h>
#endif

#if GSAGE_PLATFORM == GSAGE_APPLE
#include "OSX/Utils.h"
#endif

namespace Gsage {

  // set up mappings Gsage -> Ogre

  static const Ogre::CullingMode gCullMap[CULL_COUNT] = {
    Ogre::CULL_NONE,
    Ogre::CULL_CLOCKWISE,
    Ogre::CULL_ANTICLOCKWISE
  };

  static const Ogre::TextureFilterOptions gTFOMap[TFO_COUNT] = {
    Ogre::TFO_NONE,
    Ogre::TFO_BILINEAR,
    Ogre::TFO_TRILINEAR,
    Ogre::TFO_ANISOTROPIC
  };

  static const Ogre::SceneBlendFactor gSBFMap[SBF_COUNT] = {
    Ogre::SBF_ONE, Ogre::SBF_ZERO, Ogre::SBF_DEST_COLOUR, Ogre::SBF_SOURCE_COLOUR,
    Ogre::SBF_ONE_MINUS_DEST_COLOUR, Ogre::SBF_ONE_MINUS_SOURCE_COLOUR, Ogre::SBF_DEST_ALPHA, Ogre::SBF_SOURCE_ALPHA,
    Ogre::SBF_ONE_MINUS_DEST_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA
  };

  static const Ogre::SceneBlendOperation gSBOMap[SBO_COUNT] = {
    Ogre::SBO_ADD, Ogre::SBO_SUBTRACT, Ogre::SBO_REVERSE_SUBTRACT, Ogre::SBO_MIN,
    Ogre::SBO_MAX
  };

  static const Ogre::SceneBlendType gSBTMap[SBT_COUNT] = {
    Ogre::SBT_TRANSPARENT_ALPHA, Ogre::SBT_TRANSPARENT_COLOUR, Ogre::SBT_ADD, Ogre::SBT_MODULATE,
    Ogre::SBT_REPLACE
  };

  OgreUIGeom::OgreUIGeom(Ogre::uint32 h, const Ogre::String& material)
    : hash(h)
  {
    mRenderOp.vertexData = OGRE_NEW OgreV1::VertexData();
    mRenderOp.indexData  = OGRE_NEW OgreV1::IndexData();

    mRenderOp.vertexData->vertexCount = 0;
    mRenderOp.vertexData->vertexStart = 0;

    mRenderOp.indexData->indexCount = 0;
    mRenderOp.indexData->indexStart = 0;
#if OGRE_VERSION_MAJOR == 1
    mRenderOp.operationType             = Ogre::RenderOperation::OT_TRIANGLE_LIST;
#else
    mRenderOp.operationType             = Ogre::OT_TRIANGLE_LIST;
#endif
    mRenderOp.useIndexes                                    = true;
    mRenderOp.useGlobalInstancingVertexBufferIsAvailable    = false;

    OgreV1::VertexDeclaration* decl     = mRenderOp.vertexData->vertexDeclaration;

    // vertex declaration
    size_t offset = 0;
    decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_POSITION);
    offset += OgreV1::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0, offset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);
    offset += OgreV1::VertexElement::getTypeSize( Ogre::VET_FLOAT2 );
    decl->addElement(0, offset, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
#if OGRE_VERSION >= 0x020100
    mVertexElement2VecVec = decl->convertToV2();
#endif

    // sets overlay material
    this->setMaterial(material);
  }

  OgreUIGeom::~OgreUIGeom()
  {
    OGRE_DELETE mRenderOp.vertexData;
    OGRE_DELETE mRenderOp.indexData;
  }

  void OgreUIGeom::updateVertexData(
          const void* vertices,
          const void* indices,
          size_t numVertices,
          size_t numIndices,
          size_t vertexSize,
          size_t indexSize
  )
  {
    OgreV1::VertexBufferBinding* bind = mRenderOp.vertexData->vertexBufferBinding;

    if (bind->getBindings().empty() || bind->getBuffer(0)->getNumVertices() != numVertices)
    {
      bind->setBinding(0, OgreV1::HardwareBufferManager::getSingleton().createVertexBuffer(
            sizeof(UIVertex), numVertices, OgreV1::HardwareBuffer::HBU_WRITE_ONLY));
    }
    if (mRenderOp.indexData->indexBuffer.isNull() ||
        mRenderOp.indexData->indexBuffer->getNumIndexes() != numIndices)
    {
      OgreV1::HardwareIndexBuffer::IndexType it = OgreV1::HardwareIndexBuffer::IT_16BIT;
      switch(indexSize) {
        case 2:
          it = OgreV1::HardwareIndexBuffer::IT_16BIT;
          break;
        case 4:
          it = OgreV1::HardwareIndexBuffer::IT_32BIT;
          break;
        default:
          GSAGE_ASSERT(true, "unsupported index size: can be either 2 or 4");
          return;
      }

      mRenderOp.indexData->indexBuffer = OgreV1::HardwareBufferManager::getSingleton().createIndexBuffer(
          OgreV1::HardwareIndexBuffer::IT_16BIT, numIndices, OgreV1::HardwareBuffer::HBU_WRITE_ONLY);
    }

    // Copy all vertices
    void* vtxDst = bind->getBuffer(0)->lock(OgreV1::HardwareBuffer::HBL_DISCARD);
    void* idxDst = mRenderOp.indexData->indexBuffer->lock(OgreV1::HardwareBuffer::HBL_DISCARD);

    memcpy(vtxDst, vertices, numVertices * vertexSize);
    memcpy(idxDst, indices, numIndices * indexSize);

    mRenderOp.vertexData->vertexStart = 0;
    mRenderOp.vertexData->vertexCount = numVertices;
    mRenderOp.indexData->indexStart = 0;
    mRenderOp.indexData->indexCount = numIndices;

    bind->getBuffer(0)->unlock();
    mRenderOp.indexData->indexBuffer->unlock();
  }

  void OgreUIGeom::setMaterial(const Ogre::String& matName)
  {
    mMaterial = Ogre::MaterialManager::getSingleton().getByName( matName );
    if( mMaterial.isNull() )
    {
      OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND, "Could not find material " + matName,
          "OgreUIGeom::setMaterial");
    }

    // Won't load twice anyway
    mMaterial->load();
  }

  //-----------------------------------------------------------------------------------

  void OgreUIGeom::setMaterial(const Ogre::MaterialPtr & material)
  {
    mMaterial = material;
  }

  //-----------------------------------------------------------------------------------

  const Ogre::MaterialPtr& OgreUIGeom::getMaterial(void) const
  {
    return mMaterial;
  }

  //-----------------------------------------------------------------------------------

  void OgreUIGeom::getWorldTransforms( Ogre::Matrix4* xform ) const
  {
    *xform = Ogre::Matrix4::IDENTITY;
  }

  //-----------------------------------------------------------------------------------

  void OgreUIGeom::getRenderOperation(OgreV1::RenderOperation& op
#if OGRE_VERSION >= 0x020100
      , bool casterPass
#endif
  )
  {
    op = mRenderOp;
  }

  //-----------------------------------------------------------------------------------

  const Ogre::LightList& OgreUIGeom::getLights(void) const
  {
    static const Ogre::LightList l;
    return l;
  }

  OgreUIRenderer::OgreUIRenderer()
    : mTexUnit(0)
#if OGRE_VERSION >= 0x020100
    , mPsoCache(0)
#endif
    , mSceneMgr(0)
    , mRenderTarget(0)
  {
  }

  OgreUIRenderer::~OgreUIRenderer()
  {
#if OGRE_VERSION >= 0x020100
    if(mPsoCache != 0) {
      delete mPsoCache;
    }
#endif
  }

  UIGeomPtr OgreUIRenderer::createUIGeom() const
  {
#if OGRE_VERSION >= 0x020100
    if(!mPsoCache) {
      LOG(ERROR) << "Failed to create geom because OgreUIRenderer was not initialized";
      return 0;
    }
#endif

    Ogre::uint32 hash =
#if OGRE_VERSION >= 0x020100
      mPsoCache->getRenderableHash();
#else
      0;
#endif
    return UIGeomPtr(new OgreUIGeom(hash, mMaterialName));
  }

  void OgreUIRenderer::begin(const std::string& ctxName)
  {
    mRenderTarget = static_cast<OgreRenderSystem*>(mRender)->getRenderTarget(ctxName);
    if(!mRenderTarget) {
      return;
    }

    Ogre::Matrix4 projMatrix(2.0f / (float)mRenderTarget->getWidth(), 0.0f, 0.0f, -1.0f,
        0.0f, -2.0f / (float)mRenderTarget->getHeight(), 0.0f, 1.0f,
        0.0f, 0.0f, -1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f);

#if OGRE_VERSION_MAJOR == 2
    const Ogre::HlmsBlendblock *blendblock = mPass->getBlendblock();
    const Ogre::HlmsMacroblock *macroblock = mPass->getMacroblock();
#if OGRE_VERSION_MINOR == 0
    mSceneMgr->getDestinationRenderSystem()->_setHlmsBlendblock(blendblock);
    mSceneMgr->getDestinationRenderSystem()->_setHlmsMacroblock(macroblock);
#else
    mPsoCache->clearState();

    mPsoCache->setBlendblock(blendblock);
    mPsoCache->setMacroblock(macroblock);
    auto vertexShader = mPass->getVertexProgram();
    mPsoCache->setVertexShader(vertexShader);
    auto pixelShader = mPass->getFragmentProgram();
    mPsoCache->setPixelShader(pixelShader);

    mPsoCache->setRenderTarget(mRenderTarget->getOgreRenderTarget());
#endif
#endif
    mPass->getVertexProgramParameters()->setNamedConstant("ProjectionMatrix", projMatrix);
  }

  void OgreUIRenderer::renderUIGeom(UIGeomPtr g)
  {
    if(!mRenderTarget) {
      return;
    }

    OgreUIGeom* geom = static_cast<OgreUIGeom*>(g.get());
    if(geom->x != 0 || geom->y != 0) {
      Ogre::Matrix4 transform;
      transform.makeTrans(geom->x, geom->y, 0);
      Ogre::Matrix4 projMatrix(2.0f / (float)mRenderTarget->getWidth(), 0.0f, 0.0f, -1.0f,
          0.0f, -2.0f / (float)mRenderTarget->getHeight(), 0.0f, 1.0f,
          0.0f, 0.0f, -1.0f, 0.0f,
          0.0f, 0.0f, 0.0f, 1.0f);

      mPass->getVertexProgramParameters()->setNamedConstant("ProjectionMatrix", projMatrix * transform);
    }

    Ogre::TextureUnitState* texUnitState = mTexUnit;
    if (geom->texture) {
      OgreTexture* texture = (OgreTexture*)geom->texture.get();
      Ogre::TexturePtr tex = texture->getOgreTexture();
      if(texture->isValid() && !tex.isNull())
      {
        mTexUnit->setTexture(tex);
        setFiltering(Ogre::TFO_TRILINEAR);
      }
    } else {
      setFiltering(Ogre::TFO_NONE);
    }

    Ogre::Viewport* vp = mRenderTarget->getViewport();

    //set scissoring
    int vpLeft, vpTop, vpWidth, vpHeight;
    vp->getActualDimensions(vpLeft, vpTop, vpWidth, vpHeight);
    float factor = 1.0f;
#if GSAGE_PLATFORM == GSAGE_APPLE
    factor = GetScreenScaleFactor();
#endif
    int scLeft = geom->scLeft;
    int scTop = geom->scTop;
    int scRight = geom->scRight;
    int scBottom = geom->scBottom;

    scLeft = scLeft < 0 ? 0 : (scLeft > vpWidth ? vpWidth : scLeft);
    scRight = scRight < 0 ? 0 : (scRight > vpWidth ? vpWidth : scRight);
    scTop = scTop < 0 ? 0 : (scTop > vpHeight ? vpHeight : scTop);
    scBottom = scBottom < 0 ? 0 : (scBottom > vpHeight ? vpHeight : scBottom);
#if OGRE_VERSION_MAJOR == 1
    mSceneMgr->getDestinationRenderSystem()->setScissorTest(true, scLeft, scTop, scRight, scBottom);
#else
    float left = (float)scLeft / (float)vpWidth * factor;
    float top = (float)scTop / (float)vpHeight * factor;
    float width = (float)(scRight - scLeft) / (float)vpWidth * factor;
    float height = (float)(scBottom - scTop) / (float)vpHeight * factor;

    vp->setScissors(std::max(0.0f, left), std::max(0.0f, top), std::min(1.0f, width), std::min(1.0f, height));
    mSceneMgr->getDestinationRenderSystem()->_setViewport(vp);
#endif
    //render the object
#if OGRE_VERSION_MAJOR == 1
    mSceneMgr->_injectRenderWithPass(mPass, geom, false, false);
#elif OGRE_VERSION_MAJOR == 2

#if OGRE_VERSION_MINOR == 0
    mSceneMgr->_injectRenderWithPass(mPass, geom, 0, false, false);
#else
    OgreV1::RenderOperation renderOp;
    geom->getRenderOperation(renderOp, false);
    Ogre::VertexElement2VecVec vertexData = geom->mVertexElement2VecVec;

    mPsoCache->setVertexFormat(vertexData,
        renderOp.operationType,
        true);

    Ogre::HlmsPso *pso = mPsoCache->getPso(geom->hash, true);
    mSceneMgr->getDestinationRenderSystem()->_setPipelineStateObject(pso);
    mSceneMgr->getDestinationRenderSystem()->bindGpuProgramParameters(Ogre::GPT_VERTEX_PROGRAM,
        mPass->getVertexProgramParameters(), Ogre::GPV_GLOBAL);
    if(texUnitState) {
      mSceneMgr->getDestinationRenderSystem()->_setTextureUnitSettings(0, *texUnitState);
    }

    Ogre::v1::CbRenderOp op(renderOp);
    mSceneMgr->getDestinationRenderSystem()->_setRenderOperation(&op);
    mSceneMgr->getDestinationRenderSystem()->_render(renderOp);
#endif
#endif
  }

  void OgreUIRenderer::end()
  {
    GSAGE_ASSERT(mRenderTarget != nullptr, "OgreUIRenderer: calling end without begin");
    Ogre::Viewport* vp = mRenderTarget->getViewport();

    //reset Scissors
#if OGRE_VERSION_MAJOR == 1
    mSceneMgr->getDestinationRenderSystem()->setScissorTest(false);
#elif OGRE_VERSION_MAJOR == 2
    vp->setScissors(0, 0, 1, 1);
    mSceneMgr->getDestinationRenderSystem()->_setViewport(vp);
#endif
  }

  void OgreUIRenderer::initialize(UIRenderer::Configuration& config, RenderSystem* render, const std::string& mgrName)
  {
    UIRenderer::initialize(config, render, mgrName);
    OgreRenderSystem* rs = static_cast<OgreRenderSystem*>(render);
    mSceneMgr = rs->getSceneManager();
#if OGRE_VERSION >= 0x020100
    mPsoCache = OGRE_NEW Ogre::PsoCacheHelper(rs->getRenderSystem());
#endif

    mMaterialName = mgrName + "/material";

    Ogre::MaterialPtr overlayMaterial = Ogre::MaterialManager::getSingleton().getByName(mMaterialName);
    if(overlayMaterial.isNull()) {
      Ogre::HighLevelGpuProgramManager& mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

      Ogre::HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName("overlay/VP");
      Ogre::HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName("overlay/FP");

      Ogre::HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName("overlay/VP/D3D11");
      Ogre::HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName("overlay/FP/D3D11");

      Ogre::HighLevelGpuProgramPtr vertexShaderD3D9 = mgr.getByName("overlay/VP/D3D9");
      Ogre::HighLevelGpuProgramPtr pixelShaderD3D9 = mgr.getByName("overlay/FP/D3D9");

      Ogre::HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName("overlay/VP/GL");
      Ogre::HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName("overlay/FP/GL");

      Ogre::HighLevelGpuProgramPtr vertexShaderMetal = mgr.getByName("overlay/VP/Metal");
      Ogre::HighLevelGpuProgramPtr pixelShaderMetal = mgr.getByName("overlay/FP/Metal");

      if(vertexShaderUnified.isNull())
      {
        vertexShaderUnified = mgr.createProgram(
            "overlay/VP",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "unified",
            Ogre::GPT_VERTEX_PROGRAM);
      }

      if(pixelShaderUnified.isNull())
      {
        pixelShaderUnified = mgr.createProgram(
            "overlay/FP",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "unified",
            Ogre::GPT_FRAGMENT_PROGRAM);
      }

      Ogre::UnifiedHighLevelGpuProgram* vertexShaderPtr = static_cast<Ogre::UnifiedHighLevelGpuProgram*>(vertexShaderUnified.get());
      Ogre::UnifiedHighLevelGpuProgram* pixelShaderPtr = static_cast<Ogre::UnifiedHighLevelGpuProgram*>(pixelShaderUnified.get());

      if (vertexShaderD3D11.isNull())
      {
        vertexShaderD3D11 = mgr.createProgram("overlay/VP/D3D11",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl",
            Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderD3D11->setParameter("target", "vs_4_0");
        vertexShaderD3D11->setParameter("entry_point", "main");
        vertexShaderD3D11->setSource(UI_VS_D3D11);
        vertexShaderD3D11->load();

        vertexShaderPtr->addDelegateProgram(vertexShaderD3D11->getName());
      }

      if (pixelShaderD3D11.isNull())
      {
        pixelShaderD3D11 = mgr.createProgram(
            "overlay/FP/D3D11",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl",
            Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderD3D11->setParameter("target", "ps_4_0");
        pixelShaderD3D11->setParameter("entry_point", "main");
        pixelShaderD3D11->setSource(UI_PS_D3D11);
        pixelShaderD3D11->load();

        pixelShaderPtr->addDelegateProgram(pixelShaderD3D11->getName());
      }

      if (vertexShaderD3D9.isNull())
      {
        vertexShaderD3D9 = mgr.createProgram(
            "overlay/VP/D3D9",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl",
            Ogre::GPT_VERTEX_PROGRAM);
        vertexShaderD3D9->setParameter("target", "vs_2_0");
        vertexShaderD3D9->setParameter("entry_point", "main");
        vertexShaderD3D9->setSource(UI_VS_D3D9);
        vertexShaderD3D9->load();

        vertexShaderPtr->addDelegateProgram(vertexShaderD3D9->getName());
      }

      if (pixelShaderD3D9.isNull())
      {
        pixelShaderD3D9 = mgr.createProgram(
            "overlay/FP/D3D9",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "hlsl",
            Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderD3D9->setParameter("target", "ps_2_0");
        pixelShaderD3D9->setParameter("entry_point", "main");
        pixelShaderD3D9->setSource(UI_PS_D3D9);
        pixelShaderD3D9->load();

        pixelShaderPtr->addDelegateProgram(pixelShaderD3D9->getName());
      }

      bool useGLSL120 = rs->getRenderSystem()->getDriverVersion().major < 3 || OGRE_VERSION_MAJOR == 1;

      if (vertexShaderGL.isNull())
      {
        vertexShaderGL = mgr.createProgram(
            "overlay/VP/GL",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "glsl",
            Ogre::GPT_VERTEX_PROGRAM);

        // select appropriate vertex shader for opengl 2
        if(useGLSL120) {
          vertexShaderGL->setSource(UI_VS_GLSL120);
        } else {
          vertexShaderGL->setSource(UI_VS_GLSL150);
        }

        vertexShaderGL->load();
        vertexShaderPtr->addDelegateProgram(vertexShaderGL->getName());
      }

      if (pixelShaderGL.isNull())
      {
        pixelShaderGL = mgr.createProgram(
            "overlay/FP/GL",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "glsl",
            Ogre::GPT_FRAGMENT_PROGRAM);
        // select appropriate pixel shader for opengl 2
        if(useGLSL120) {
          pixelShaderGL->setSource(UI_PS_GLSL120);
        } else {
          pixelShaderGL->setSource(UI_PS_GLSL150);
        }
        pixelShaderGL->load();
        pixelShaderGL->setParameter("sampler0", "int 0");

        pixelShaderPtr->addDelegateProgram(pixelShaderGL->getName());
      }

      if (vertexShaderMetal.isNull())
      {
        vertexShaderMetal = mgr.createProgram(
            "overlay/VP/Metal",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "metal",
            Ogre::GPT_VERTEX_PROGRAM);

        vertexShaderMetal->setSource(UI_VS_METAL);
        vertexShaderMetal->load();

        vertexShaderPtr->addDelegateProgram(vertexShaderMetal->getName());
      }

      if (pixelShaderMetal.isNull())
      {
        pixelShaderMetal = mgr.createProgram(
            "overlay/FP/Metal",
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            "metal",
            Ogre::GPT_FRAGMENT_PROGRAM);
        pixelShaderMetal->setSource(UI_PS_METAL);
        pixelShaderMetal->setParameter("shader_reflection_pair_hint", "overlay/VP/Metal");
        pixelShaderMetal->load();

        pixelShaderPtr->addDelegateProgram(pixelShaderMetal->getName());
      }
      overlayMaterial = Ogre::MaterialManager::getSingleton().create(mMaterialName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    }

    mPass = overlayMaterial->getTechnique(0)->getPass(0);
    mPass->setFragmentProgram("overlay/FP");
    mPass->setVertexProgram("overlay/VP");

    Ogre::SceneBlendType blendType = gSBTMap[config.blendType];
    Ogre::CullingMode cullMode = gCullMap[config.cullMode];
    Ogre::SceneBlendOperation blendOperation = gSBOMap[config.blendOperation];
    Ogre::SceneBlendOperation blendOperationAlpha = gSBOMap[config.blendOperationAlpha];
    Ogre::SceneBlendFactor srcBlendFactor = gSBFMap[config.srcBlendFactor];
    Ogre::SceneBlendFactor dstBlendFactor = gSBFMap[config.dstBlendFactor];
    Ogre::SceneBlendFactor srcBlendFactorAlpha = gSBFMap[config.srcBlendFactorAlpha];
    Ogre::SceneBlendFactor dstBlendFactorAlpha = gSBFMap[config.dstBlendFactorAlpha];

#if OGRE_VERSION < 0x020100
    mPass->setCullingMode(cullMode);
    mPass->setDepthFunction(Ogre::CMPF_ALWAYS_PASS);
    mPass->setLightingEnabled(false);
    mPass->setSceneBlending(blendType);
    mPass->setSeparateSceneBlendingOperation(
        blendOperation,
        blendOperationAlpha
    );
    mPass->setSeparateSceneBlending(
        srcBlendFactor,
        dstBlendFactor,
        srcBlendFactorAlpha,
        dstBlendFactorAlpha
    );
#else
    Ogre::HlmsMacroblock macroblock;
    macroblock.mCullMode = cullMode;
    macroblock.mDepthFunc = Ogre::CMPF_ALWAYS_PASS;
    macroblock.mDepthWrite = false;
    macroblock.mDepthCheck = false;
    macroblock.mAllowGlobalDefaults = false;
    macroblock.mScissorTestEnabled = true;
    mPass->setMacroblock(macroblock);

    Ogre::HlmsBlendblock blendblock;
    blendblock.setBlendType(blendType);
    blendblock.mSeparateBlend = true;
    blendblock.mAllowGlobalDefaults = false;
    blendblock.mIsTransparent = true;
    blendblock.mBlendOperation = blendOperation;
    blendblock.mBlendOperationAlpha = blendOperationAlpha;
    blendblock.mSourceBlendFactor = srcBlendFactor;
    blendblock.mDestBlendFactor = dstBlendFactor;
    blendblock.mSourceBlendFactorAlpha = srcBlendFactorAlpha;
    blendblock.mDestBlendFactorAlpha = dstBlendFactorAlpha;
    mPass->setBlendblock(blendblock);
#endif
    Ogre::TextureUnitState* texUnit = mPass->createTextureUnitState();
    mTexUnit = texUnit;
    setFiltering(gTFOMap[config.texFiltering]);
  }

  void OgreUIRenderer::setFiltering(Ogre::TextureFilterOptions mode)
  {
#if OGRE_VERSION < 0x020100
    mTexUnit->setTextureFiltering(mode);
#else
    Ogre::HlmsSamplerblock samplerblock;
    samplerblock.setFiltering(mode);
    mTexUnit->setSamplerblock(samplerblock);
#endif
  }

}
