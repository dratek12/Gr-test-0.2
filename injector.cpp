#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstdlib>

static DWORD FindPid(const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe{}; pe.dwSize = sizeof(pe);
    DWORD pid = 0;
    if (Process32FirstW(snap, &pe)) do {
        if (!_wcsicmp(pe.szExeFile, name)) { pid = pe.th32ProcessID; break; }
    } while (Process32NextW(snap, &pe));
    CloseHandle(snap);
    return pid;
}

static bool ModulePresent(DWORD pid, const wchar_t* name) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me{}; me.dwSize = sizeof(me);
    bool found = false;
    if (Module32FirstW(snap, &me)) do {
        if (!_wcsicmp(me.szModule, name)) { found = true; break; }
    } while (Module32NextW(snap, &me));
    CloseHandle(snap);
    return found;
}

static bool Inject(HANDLE proc, DWORD pid, const wchar_t* dll) {
    if (ModulePresent(pid, L"Cs2Esp.dll")) {
        printf("modul uz v procesu je, neinjektuji znovu\n");
        return true;
    }

    const SIZE_T bytes = (wcslen(dll) + 1) * sizeof(wchar_t);

    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { printf("VirtualAllocEx: %lu\n", GetLastError()); return false; }

    if (!WriteProcessMemory(proc, remote, dll, bytes, nullptr)) {
        printf("WriteProcessMemory: %lu\n", GetLastError());
        VirtualFreeEx(proc, remote, 0, MEM_RELEASE);
        return false;
    }

    wchar_t back[MAX_PATH]{};
    SIZE_T read = 0;
    ReadProcessMemory(proc, remote, back, bytes, &read);
    printf("zapsano: %ls\n", back);
    if (wcsicmp(back, dll) != 0) { printf("cesta nesedi\n"); return false; }

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    auto load = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel, "LoadLibraryW"));
    HANDLE t = CreateRemoteThread(proc, nullptr, 0, load, remote, 0, nullptr);
    if (!t) { printf("CreateRemoteThread: %lu\n", GetLastError()); return false; }

    WaitForSingleObject(t, 15000);
    DWORD code = 0;
    GetExitCodeThread(t, &code);
    CloseHandle(t);
    printf("exit kod vlakna: %lu\n", code);

    bool present = ModulePresent(pid, L"Cs2Esp.dll");
    printf("modul v procesu: %s\n", present ? "ANO" : "NE");
    if (!present) printf("kdyz je NE, mrkni do konzole CS2 na 'File verification failure'\n");
    return present;
}

static bool Eject(HANDLE proc, DWORD pid) {
    HMODULE remote = nullptr;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap != INVALID_HANDLE_VALUE) {
        MODULEENTRY32W me{}; me.dwSize = sizeof(me);
        if (Module32FirstW(snap, &me)) do {
            if (!_wcsicmp(me.szModule, L"Cs2Esp.dll")) { remote = me.hModule; break; }
        } while (Module32NextW(snap, &me));
        CloseHandle(snap);
    }
    if (!remote) { printf("modul v procesu neni\n"); return false; }

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    auto freeLib = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel, "FreeLibrary"));
    HANDLE t = CreateRemoteThread(proc, nullptr, 0, freeLib, remote, 0, nullptr);
    if (!t) { printf("CreateRemoteThread: %lu\n", GetLastError()); return false; }
    WaitForSingleObject(t, 5000);
    CloseHandle(t);
    printf("odinjektovano\n");
    return true;
}

int main(int argc, char** argv) {
    SetConsoleTitleW(L"Cs2Esp injector");

    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    wchar_t* slash = wcsrchr(exe, L'\\');
    if (slash) *slash = 0;

    bool test = argc > 1 && !_stricmp(argv[1], "test");
    bool eject = argc > 1 && !_stricmp(argv[1], "eject");

    wchar_t dll[MAX_PATH]{};
    if (argc > 2 && argv[2][0])
        MultiByteToWideChar(CP_ACP, 0, argv[2], -1, dll, MAX_PATH);
    else
        swprintf_s(dll, L"%s\\Cs2Esp.dll", exe);

    printf("DLL: %ls\n", dll);
    if (GetFileAttributesW(dll) == INVALID_FILE_ATTRIBUTES) {
        printf("\nCHYBA: DLL nenalezena.\n");
        printf("Zkopiruj Cs2Esp.dll do slozky hry a spust:\n");
        printf("  Injector.exe inject \"C:\\...\\game\\csgo\\bin\\win64\\Cs2Esp.dll\"\n");
        system("pause");
        return 1;
    }

    if (test) {
        printf("\n--- test proti notepad.exe ---\n");
        STARTUPINFOW si{ sizeof(si) };
        PROCESS_INFORMATION pi{};
        wchar_t cmd[] = L"notepad.exe";
        if (!CreateProcessW(nullptr, cmd, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
            printf("CreateProcess selhal\n"); system("pause"); return 2;
        }
        Sleep(1500);
        bool ok = Inject(pi.hProcess, pi.dwProcessId, dll);
        printf(ok ? "\nV NOTEPADU SE NACETLA\n" : "\nV NOTEPADU SE TAKY NENACETLA\n");
        TerminateProcess(pi.hProcess, 0);
        system("pause");
        return 0;
    }

    DWORD pid = FindPid(L"cs2.exe");
    if (!pid) { printf("cs2.exe nebezi\n"); system("pause"); return 3; }
    printf("pid %lu\n", pid);

    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        printf("OpenProcess: %lu\n", GetLastError());
        proc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!proc) { printf("spust jako spravce\n"); system("pause"); return 4; }
    }

    bool ok = eject ? Eject(proc, pid) : Inject(proc, pid, dll);
    if (!eject) printf(ok ? "\nOK, zkus Insert\n" : "\nselhalo\n");

    CloseHandle(proc);
    system("pause");
    return 0;
}