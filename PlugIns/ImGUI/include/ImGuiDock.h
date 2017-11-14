#ifndef __IMGUI_DOCK__
#define __IMGUI_DOCK__

#include "imgui.h"
#include "DataProxy.h"
// based on https://github.com/nem0/LumixEngine/blob/master/external/imgui/imgui_dock.h

#ifdef __cplusplus
extern "C" {
#else
typedef struct ImVec2 ImVec2;
typedef int ImGuiWindowFlags;
#endif

#define TABBAR_PADDING_X 20.0f
#define TABBAR_PADDING_Y 6.0f

typedef enum ImGuiDockSlot {
    ImGuiDockSlot_Left,
    ImGuiDockSlot_Right,
    ImGuiDockSlot_Top,
    ImGuiDockSlot_Bottom,
    ImGuiDockSlot_Tab,

    ImGuiDockSlot_Float,
    ImGuiDockSlot_None
} ImGuiDockSlot;

extern void igBeginWorkspace();
extern void igEndWorkspace();
extern void igShutdownDock();
extern void igSetNextDock(ImGuiDockSlot slot);
extern bool igBeginDock(const char* label, bool* opened, ImGuiWindowFlags extra_flags);
extern void igEndDock();
extern void igSetDockActive();
extern void igDockDebugWindow();

#ifdef __cplusplus
}
#endif

extern Gsage::DataProxy getDockState();
extern void setDockState(const Gsage::DataProxy& setDockState);

#endif
