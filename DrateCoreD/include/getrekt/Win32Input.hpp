#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "GetRektUI.hpp"
namespace getrekt {
// Call BEFORE the platform ImGui handler and BEFORE application shortcuts.
inline bool Win32Input(UI& ui,UINT msg,WPARAM w,LPARAM) {
    if(msg==WM_KILLFOCUS){ui.setFocused(false);return false;}
    if(msg==WM_SETFOCUS){ui.setFocused(true);return false;}
    int key=0;bool down=false;
    switch(msg){
    case WM_KEYDOWN:case WM_SYSKEYDOWN:key=(int)w;down=true;break;
    case WM_KEYUP:case WM_SYSKEYUP:key=(int)w;break;
    case WM_LBUTTONDOWN:case WM_LBUTTONUP:key=1;down=msg==WM_LBUTTONDOWN;break;
    case WM_RBUTTONDOWN:case WM_RBUTTONUP:key=2;down=msg==WM_RBUTTONDOWN;break;
    case WM_MBUTTONDOWN:case WM_MBUTTONUP:key=4;down=msg==WM_MBUTTONDOWN;break;
    case WM_XBUTTONDOWN:case WM_XBUTTONUP:key=HIWORD(w)==XBUTTON1?5:6;down=msg==WM_XBUTTONDOWN;break;
    }
    const bool consumed=key&&ui.keyEvent(key,down);
    return consumed;
}
}
