#include <imgui.h>
#include "ImguiRenderable.h"
#include "ImguiManager.h"

#include <OgreMaterialManager.h>
#include <OgreMesh.h>
#include <OgreMeshManager.h>
#include <OgreSubMesh.h>
#include <OgreTexture.h>
#include <OgreTextureManager.h>
#include <OgreString.h>
#include <OgreStringConverter.h>
#include <OgreViewport.h>
#include <OgreHighLevelGpuProgramManager.h>
#include <OgreHighLevelGpuProgram.h>
#include <OgreUnifiedHighLevelGpuProgram.h>
#include <OgreRoot.h>
#include <OgreTechnique.h>
#include <OgreViewport.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRenderTarget.h>

using namespace Ogre;


template<> ImguiManager* Singleton<ImguiManager>::msSingleton = 0;

void ImguiManager::createSingleton()
{
    if(!msSingleton)
    {
        msSingleton = new ImguiManager();
    }
}
ImguiManager* ImguiManager::getSingletonPtr(void)
{
    createSingleton();
    return msSingleton;
}
ImguiManager& ImguiManager::getSingleton(void)
{  
    createSingleton();
    return ( *msSingleton );  
}

ImguiManager::ImguiManager()
:mSceneMgr(0)
,mLastRenderedFrame(-1)
,OIS::MouseListener()
,OIS::KeyListener()
,mKeyInput(0)
,mMouseInput(0)
{

}
ImguiManager::~ImguiManager()
{
    while(mRenderables.size()>0)
    {
        delete mRenderables.back();
        mRenderables.pop_back();
    }
    mSceneMgr->removeRenderQueueListener(this);
}
void ImguiManager::init(Ogre::SceneManager * mgr,OIS::Keyboard* keyInput, OIS::Mouse* mouseInput)
{
    mSceneMgr  = mgr;
    mMouseInput= mouseInput;
    mKeyInput = keyInput;

    mSceneMgr->addRenderQueueListener(this);
    ImGuiIO& io = ImGui::GetIO();

    io.KeyMap[ImGuiKey_Tab] = OIS::KC_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
    io.KeyMap[ImGuiKey_LeftArrow] = OIS::KC_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = OIS::KC_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow] = OIS::KC_UP;
    io.KeyMap[ImGuiKey_DownArrow] = OIS::KC_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = OIS::KC_PGUP;
    io.KeyMap[ImGuiKey_PageDown] = OIS::KC_PGDOWN;
    io.KeyMap[ImGuiKey_Home] = OIS::KC_HOME;
    io.KeyMap[ImGuiKey_End] = OIS::KC_END;
    io.KeyMap[ImGuiKey_Delete] = OIS::KC_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = OIS::KC_BACK;
    io.KeyMap[ImGuiKey_Enter] = OIS::KC_RETURN;
    io.KeyMap[ImGuiKey_Escape] = OIS::KC_ESCAPE;
    io.KeyMap[ImGuiKey_A] = OIS::KC_A;
    io.KeyMap[ImGuiKey_C] = OIS::KC_C;
    io.KeyMap[ImGuiKey_V] = OIS::KC_V;
    io.KeyMap[ImGuiKey_X] = OIS::KC_X;
    io.KeyMap[ImGuiKey_Y] = OIS::KC_Y;
    io.KeyMap[ImGuiKey_Z] = OIS::KC_Z;

    createFontTexture();
    createMaterial();
}
 //Inherhited from OIS::MouseListener
bool ImguiManager::mouseMoved( const OIS::MouseEvent &arg )
{

    ImGuiIO& io = ImGui::GetIO();

    io.MousePos.x = arg.state.X.abs;
    io.MousePos.y = arg.state.Y.abs;

    return true;
}
bool ImguiManager::mousePressed( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if(id<5)
    {
        io.MouseDown[id] = true;
    }
    return true;
}
bool ImguiManager::mouseReleased( const OIS::MouseEvent &arg, OIS::MouseButtonID id )
{
    ImGuiIO& io = ImGui::GetIO();
    if(id<5)
    {
        io.MouseDown[id] = false;
    }
    return true;
}
//Inherhited from OIS::KeyListener
bool ImguiManager::keyPressed( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = true;
    

    if(arg.text>0)
    {
        io.AddInputCharacter((unsigned short)arg.text);
    }

    return true;
}
bool ImguiManager::keyReleased( const OIS::KeyEvent &arg )
{
    ImGuiIO& io = ImGui::GetIO();
    io.KeysDown[arg.key] = false;
    return true;
}
void ImguiManager::updateVertexData()
{
    int currentFrame = ImGui::GetFrameCount();
    if(currentFrame == mLastRenderedFrame)
    {
        return ;
    }
    mLastRenderedFrame=currentFrame;


    ImDrawData* draw_data = ImGui::GetDrawData();
    while(mRenderables.size()<draw_data->CmdListsCount)
    {
        mRenderables.push_back(new Ogre::ImGUIRenderable());
    }
    while(mRenderables.size()>draw_data->CmdListsCount)
    {
        delete mRenderables.back();
        mRenderables.pop_back();
    }
    unsigned int index=0;
    for(std::list<ImGUIRenderable*>::iterator it = mRenderables.begin();it!=mRenderables.end();++it,++index)
    {
        (*it)->updateVertexData(draw_data,index);
    }

}
//-----------------------------------------------------------------------------------
void ImguiManager::renderQueueEnded(uint8 queueGroupId, const String& invocation,bool& repeatThisInvocation)
{
    if(queueGroupId == Ogre::RENDER_QUEUE_OVERLAY && invocation !="SHADOWS")
    {
        
        Ogre::Viewport* vp = Ogre::Root::getSingletonPtr()->getRenderSystem()->_getViewport();

        if(vp != NULL && vp->getTarget()->isPrimary())
        {
            if (vp->getOverlaysEnabled())
            {
                if(mFrameEnded) return;

                mFrameEnded=true;
                ImGui::Render();
                updateVertexData();
                ImGuiIO& io = ImGui::GetIO();
                Ogre::Matrix4 projMatrix(2.0f/io.DisplaySize.x, 0.0f,                   0.0f,-1.0f,
                                 0.0f,                 -2.0f/io.DisplaySize.y,  0.0f, 1.0f,
                                 0.0f,                  0.0f,                  -1.0f, 0.0f,
                                 0.0f,                  0.0f,                   0.0f, 1.0f);

                mPass->getVertexProgramParameters()->setNamedConstant("ProjectionMatrix",projMatrix);
                for(std::list<ImGUIRenderable*>::iterator it = mRenderables.begin();it!=mRenderables.end();++it)
                {
                    mSceneMgr->_injectRenderWithPass(mPass, (*it), false, false);
                }
            }
        }
    }
}
//-----------------------------------------------------------------------------------
void ImguiManager::createMaterial()
{
    static const char* vertexShaderSrcD3D11 =
    {
    "cbuffer vertexBuffer : register(b0) \n"
    "{\n"
    "float4x4 ProjectionMatrix; \n"
    "};\n"
    "struct VS_INPUT\n"
    "{\n"
    "float2 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
    "PS_INPUT output;\n"
    "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
    "output.col = input.col;\n"
    "output.uv  = input.uv;\n"
    "return output;\n"
    "}"
    };

    static const char* pixelShaderSrcD3D11 =
    {
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler sampler0;\n"
    "Texture2D texture0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \n"
    "return out_col; \n"
    "}"
    };
	static const char* vertexShaderSrcD3D9 =
    {
    "uniform float4x4 ProjectionMatrix; \n"
    "struct VS_INPUT\n"
    "{\n"
    "float2 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "PS_INPUT main(VS_INPUT input)\n"
    "{\n"
    "PS_INPUT output;\n"
    "output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\n"
    "output.col = input.col;\n"
    "output.uv  = input.uv;\n"
    "return output;\n"
    "}"
    };
   
    static const char* pixelShaderSrcSrcD3D9 =
     {
    "struct PS_INPUT\n"
    "{\n"
    "float4 pos : SV_POSITION;\n"
    "float4 col : COLOR0;\n"
    "float2 uv  : TEXCOORD0;\n"
    "};\n"
    "sampler2D sampler0;\n"
    "\n"
    "float4 main(PS_INPUT input) : SV_Target\n"
    "{\n"
    "float4 out_col = input.col.bgra * tex2D(sampler0, input.uv); \n"
    "return out_col; \n"
    "}"
    };

    static const char* vertexShaderSrcGLSL150 =
    {
      "#version 150\n"
      "uniform mat4 ProjectionMatrix; \n"
      "in vec2 vertex;\n"
      "in vec2 uv0;\n"
      "in vec4 colour;\n"
      "out vec2 Texcoord;\n"
      "out vec4 ocol;\n"
      "void main()\n"
      "{\n"
      "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
      "Texcoord  = uv0;\n"
      "ocol = colour;\n"
      "}"
    };

    static const char* vertexShaderSrcGLSL120 =
    {
      "#version 120\n"
      "uniform mat4 ProjectionMatrix; \n"
      "attribute vec2 vertex;\n"
      "attribute vec2 uv0;\n"
      "attribute vec4 colour;\n"
      "varying vec2 Texcoord;\n"
      "varying vec4 ocol;\n"
      "void main()\n"
      "{\n"
        "gl_Position = ProjectionMatrix* vec4(vertex.xy, 0.f, 1.f);\n"
        "Texcoord  = uv0;\n"
        "ocol = colour;\n"
      "}"
    };

    static const char* pixelShaderSrcGLSL150 =
    {
      "#version 150\n"
      "in vec2 Texcoord;\n"
      "in vec4 col;\n"
      "uniform sampler2D sampler0;\n"
      "out vec4 out_col;\n"
      "void main()\n"
      "{\n"
        "out_col = col * texture(sampler0, Texcoord); \n"
      "}"
    };

    static const char* pixelShaderSrcGLSL120 =
    {
      "#version 120\n"
      "varying vec2 Texcoord;\n"
      "varying vec4 ocol;\n"
      "uniform sampler2D sampler0;\n"
      "varying vec4 out_col;\n"
      "void main()\n"
      "{\n"
        "gl_FragColor = ocol * texture2D(sampler0, Texcoord); \n"
      "}"
    };

    //create the default shadows material
    Ogre::HighLevelGpuProgramManager& mgr = Ogre::HighLevelGpuProgramManager::getSingleton();

    Ogre::HighLevelGpuProgramPtr vertexShaderUnified = mgr.getByName("imgui/VP");
    Ogre::HighLevelGpuProgramPtr pixelShaderUnified = mgr.getByName("imgui/FP");

    Ogre::HighLevelGpuProgramPtr vertexShaderD3D11 = mgr.getByName("imgui/VP/D3D11");
    Ogre::HighLevelGpuProgramPtr pixelShaderD3D11 = mgr.getByName("imgui/FP/D3D11");

    Ogre::HighLevelGpuProgramPtr vertexShaderD3D9 = mgr.getByName("imgui/VP/D3D9");
    Ogre::HighLevelGpuProgramPtr pixelShaderD3D9 = mgr.getByName("imgui/FP/D3D9");

    Ogre::HighLevelGpuProgramPtr vertexShaderGL = mgr.getByName("imgui/VP/GL");
    Ogre::HighLevelGpuProgramPtr pixelShaderGL = mgr.getByName("imgui/FP/GL");

    if(vertexShaderUnified.isNull())
    {
      vertexShaderUnified = mgr.createProgram("imgui/VP",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"unified",GPT_VERTEX_PROGRAM);
    }
    if(pixelShaderUnified.isNull())
    {
      pixelShaderUnified = mgr.createProgram("imgui/FP",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,"unified",GPT_FRAGMENT_PROGRAM);
    }

    UnifiedHighLevelGpuProgram* vertexShaderPtr = static_cast<UnifiedHighLevelGpuProgram*>(vertexShaderUnified.get());
    UnifiedHighLevelGpuProgram* pixelShaderPtr = static_cast<UnifiedHighLevelGpuProgram*>(pixelShaderUnified.get());

    if (vertexShaderD3D11.isNull())
    {
      vertexShaderD3D11 = mgr.createProgram("imgui/VP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "hlsl", Ogre::GPT_VERTEX_PROGRAM);
      vertexShaderD3D11->setParameter("target", "vs_4_0");
      vertexShaderD3D11->setParameter("entry_point", "main");
      vertexShaderD3D11->setSource(vertexShaderSrcD3D11);
      vertexShaderD3D11->load();

      vertexShaderPtr->addDelegateProgram(vertexShaderD3D11->getName());
    }
    if (pixelShaderD3D11.isNull())
    {
      pixelShaderD3D11 = mgr.createProgram("imgui/FP/D3D11", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "hlsl", Ogre::GPT_FRAGMENT_PROGRAM);
      pixelShaderD3D11->setParameter("target", "ps_4_0");
      pixelShaderD3D11->setParameter("entry_point", "main");
      pixelShaderD3D11->setSource(pixelShaderSrcD3D11);
      pixelShaderD3D11->load();

      pixelShaderPtr->addDelegateProgram(pixelShaderD3D11->getName());
    }
    if (vertexShaderD3D9.isNull())
    {
      vertexShaderD3D9 = mgr.createProgram("imgui/VP/D3D9", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "hlsl", Ogre::GPT_VERTEX_PROGRAM);
      vertexShaderD3D9->setParameter("target", "vs_2_0");
      vertexShaderD3D9->setParameter("entry_point", "main");
      vertexShaderD3D9->setSource(vertexShaderSrcD3D9);
      vertexShaderD3D9->load();

      vertexShaderPtr->addDelegateProgram(vertexShaderD3D9->getName());
    }
    if (pixelShaderD3D9.isNull())
    {
      pixelShaderD3D9 = mgr.createProgram("imgui/FP/D3D9", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "hlsl", Ogre::GPT_FRAGMENT_PROGRAM);
      pixelShaderD3D9->setParameter("target", "ps_2_0");
      pixelShaderD3D9->setParameter("entry_point", "main");
      pixelShaderD3D9->setSource(pixelShaderSrcSrcD3D9);
      pixelShaderD3D9->load();

      pixelShaderPtr->addDelegateProgram(pixelShaderD3D9->getName());
    }

    Ogre::RenderSystem* render = Ogre::Root::getSingletonPtr()->getRenderSystem();

    if (vertexShaderGL.isNull())
    {
      vertexShaderGL = mgr.createProgram("imgui/VP/GL", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "glsl", Ogre::GPT_VERTEX_PROGRAM);

      // select appropriate vertex shader for opengl 2
      if(render->getDriverVersion().major == 2) {
        vertexShaderGL->setSource(vertexShaderSrcGLSL120);
      } else {
        vertexShaderGL->setSource(vertexShaderSrcGLSL150);
      }

      vertexShaderGL->load();
      vertexShaderPtr->addDelegateProgram(vertexShaderGL->getName());
    }

    if (pixelShaderGL.isNull())
    {
      pixelShaderGL = mgr.createProgram("imgui/FP/GL", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
          "glsl", Ogre::GPT_FRAGMENT_PROGRAM);
      // select appropriate pixel shader for opengl 2
      if(render->getDriverVersion().major == 2) {
        pixelShaderGL->setSource(pixelShaderSrcGLSL120);
      } else {
        pixelShaderGL->setSource(pixelShaderSrcGLSL150);
      }
      pixelShaderGL->load();
      pixelShaderGL->setParameter("sampler0","int 0");

      pixelShaderPtr->addDelegateProgram(pixelShaderGL->getName());
    }

    Ogre::MaterialPtr imguiMaterial = Ogre::MaterialManager::getSingleton().create("imgui/material", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
    mPass = imguiMaterial->getTechnique(0)->getPass(0);
    mPass->setFragmentProgram("imgui/FP");
    mPass->setVertexProgram("imgui/VP");
    mPass->setCullingMode(CULL_NONE);
    mPass->setDepthFunction(Ogre::CMPF_ALWAYS_PASS);
    mPass->setLightingEnabled(false);
    mPass->setSceneBlending(Ogre::SBT_TRANSPARENT_ALPHA);
    mPass->setSeparateSceneBlendingOperation(Ogre::SBO_ADD,Ogre::SBO_ADD);
    mPass->setSeparateSceneBlending(Ogre::SBF_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA,Ogre::SBF_ONE_MINUS_SOURCE_ALPHA,Ogre::SBF_ZERO);


    //mPass->getFragmentProgramParameters()->setNamedConstant("sampler0",0);
    mPass->createTextureUnitState()->setTextureName("ImguiFontTex");
}

void ImguiManager::createFontTexture()
{
  // Build texture atlas
  ImGuiIO& io = ImGui::GetIO();

  ImGuiStyle& style = ImGui::GetStyle();

  style.WindowPadding            = ImVec2(15, 15);
  style.WindowRounding           = 5.0f;
  style.FramePadding             = ImVec2(5, 5);
  style.FrameRounding            = 4.0f;
  style.ItemSpacing              = ImVec2(12, 8);
  style.ItemInnerSpacing         = ImVec2(8, 6);
  style.IndentSpacing            = 25.0f;
  style.ScrollbarSize            = 15.0f;
  style.ScrollbarRounding        = 9.0f;
  style.GrabMinSize              = 5.0f;
  style.GrabRounding             = 3.0f;

  style.Colors[ImGuiCol_Text]                  = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.40f, 0.39f, 0.38f, 0.77f);
  style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.92f, 0.91f, 0.88f, 0.70f);
  style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(1.00f, 0.98f, 0.95f, 0.58f);
  style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.92f, 0.91f, 0.88f, 0.92f);
  style.Colors[ImGuiCol_Border]                = ImVec4(0.84f, 0.83f, 0.80f, 0.65f);
  style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.92f, 0.91f, 0.88f, 0.00f);
  style.Colors[ImGuiCol_FrameBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.99f, 1.00f, 0.40f, 0.78f);
  style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.26f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_TitleBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(1.00f, 0.98f, 0.95f, 0.75f);
  style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.25f, 1.00f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(1.00f, 0.98f, 0.95f, 0.47f);
  style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.00f, 0.00f, 0.00f, 0.21f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.90f, 0.91f, 0.00f, 0.78f);
  style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_ComboBg]               = ImVec4(1.00f, 0.98f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.25f, 1.00f, 0.00f, 0.80f);
  style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
  style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_Button]                = ImVec4(0.00f, 0.00f, 0.00f, 0.14f);
  style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.99f, 1.00f, 0.22f, 0.86f);
  style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_Header]                = ImVec4(0.25f, 1.00f, 0.00f, 0.76f);
  style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.25f, 1.00f, 0.00f, 0.86f);
  style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_Column]                = ImVec4(0.00f, 0.00f, 0.00f, 0.32f);
  style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
  style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.00f, 0.00f, 0.00f, 0.04f);
  style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.25f, 1.00f, 0.00f, 0.78f);
  style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.40f, 0.39f, 0.38f, 0.16f);
  style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.40f, 0.39f, 0.38f, 0.39f);
  style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.40f, 0.39f, 0.38f, 1.00f);
  style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.40f, 0.39f, 0.38f, 0.63f);
  style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.25f, 1.00f, 0.00f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.25f, 1.00f, 0.00f, 0.43f);
  style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(1.00f, 0.98f, 0.95f, 0.73f);

  unsigned char* pixels;
  int width, height;
  io.Fonts->Clear();
  io.Fonts->AddFontFromFileTTF("../Resources/fonts/OpenSans-Light.ttf", 16);
  io.Fonts->AddFontFromFileTTF("../Resources/fonts/OpenSans-Regular.ttf", 16);
  io.Fonts->AddFontFromFileTTF("../Resources/fonts/OpenSans-Light.ttf", 32);
  io.Fonts->AddFontFromFileTTF("../Resources/fonts/OpenSans-Regular.ttf", 11);
  io.Fonts->AddFontFromFileTTF("../Resources/fonts/consolas.ttf", 11);
  io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
  io.Fonts->Build();

  mFontTex = TextureManager::getSingleton().createManual("ImguiFontTex",Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,TEX_TYPE_2D,width,height,1,1,PF_R8G8B8A8);

  const PixelBox & lockBox = mFontTex->getBuffer()->lock(Image::Box(0, 0, width, height), HardwareBuffer::HBL_DISCARD);
  size_t texDepth = PixelUtil::getNumElemBytes(lockBox.format);

  memcpy(lockBox.data,pixels, width*height*texDepth);
  mFontTex->getBuffer()->unlock();
}

void ImguiManager::newFrame(float deltaTime,const Ogre::Rect & windowRect)
{
  ImGuiIO& io = ImGui::GetIO();
  mFrameEnded=false;
  io.DeltaTime = deltaTime;

  // Setup display size (every frame to accommodate for window resizing)
  io.DisplaySize = ImVec2((float)(windowRect.right - windowRect.left), (float)(windowRect.bottom - windowRect.top));

  if(mKeyInput != 0) {
    // Read keyboard modifiers inputs
    io.KeyCtrl = mKeyInput->isKeyDown(OIS::KC_LCONTROL);
    io.KeyShift = mKeyInput->isKeyDown(OIS::KC_LSHIFT);
    io.KeyAlt = mKeyInput->isKeyDown(OIS::KC_LMENU);
    io.KeySuper = false;
  }

  // Start the frame
  ImGui::NewFrame();
}
