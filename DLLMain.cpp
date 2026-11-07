#include "Global.h"
#include "Hooks.h"

static DWORD WINAPI MainThread(LPVOID) {
    Sleep(200);
    G::Log("attach pid=%lu", GetCurrentProcessId());
    InitializeHooks();
    while (!G::Unload) Sleep(500);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE mod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        G::Module = mod;
        DisableThreadLibraryCalls(mod);
        CreateThread(nullptr, 0, MainThread, nullptr, 0, nullptr);
    }
    return TRUE;
}