#include "Misc.h"
#include "Global.h"

static float gLastZ = 0.f;

void RunMisc() {
    auto& c = G::Ui.state.cs2;
    if (!c.bhop || !G::LocalAlive || !Sdk::Foreground()) { gLastZ = G::LocalOrigin.z; return; }
    if (!(GetAsyncKeyState(VK_SPACE) & 0x8000)) { gLastZ = G::LocalOrigin.z; return; }

    if (std::fabs(G::LocalOrigin.z - gLastZ) < 1.5f) {
        INPUT in[2]{};
        in[0].type = INPUT_KEYBOARD; in[0].ki.wVk = VK_SPACE; in[0].ki.dwFlags = KEYEVENTF_KEYUP;
        in[1].type = INPUT_KEYBOARD; in[1].ki.wVk = VK_SPACE;
        SendInput(2, in, sizeof(INPUT));
    }
    gLastZ = G::LocalOrigin.z;
}