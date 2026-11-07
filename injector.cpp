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

static const char* MachineOf(const wchar_t* path) {
    HANDLE f = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (f == INVALID_HANDLE_VALUE) return "neexistuje";

    IMAGE_DOS_HEADER dos{};
    DWORD got = 0;
    if (!ReadFile(f, &dos, sizeof(dos), &got, nullptr) || dos.e_magic != IMAGE_DOS_SIGNATURE) {
        CloseHandle(f); return "neni PE";
    }
    SetFilePointer(f, dos.e_lfanew, nullptr, FILE_BEGIN);

    DWORD sig = 0;
    if (!ReadFile(f, &sig, sizeof(sig), &got, nullptr) || sig != IMAGE_NT_SIGNATURE) {
        CloseHandle(f); return "neni PE";
    }
    IMAGE_FILE_HEADER fh{};
    if (!ReadFile(f, &fh, sizeof(fh), &got, nullptr)) { CloseHandle(f); return "chyba cteni"; }
    CloseHandle(f);

    if (fh.Machine == IMAGE_FILE_MACHINE_AMD64) return "x64";
    if (fh.Machine == IMAGE_FILE_MACHINE_I386) return "x86";
    return "jina arch";
}

int main() {
    SetConsoleTitleW(L"Cs2Esp injector");

    wchar_t exe[MAX_PATH]{};
    GetModuleFileNameW(nullptr, exe, MAX_PATH);
    wchar_t* slash = wcsrchr(exe, L'\\');
    if (slash) *slash = 0;

    wchar_t dll[MAX_PATH]{};
    swprintf_s(dll, L"%s\\Cs2Esp.dll", exe);

    printf("injector arch: %s\n", sizeof(void*) == 8 ? "x64" : "x86");
    printf("DLL: %ls\n", dll);

    if (GetFileAttributesW(dll) == INVALID_FILE_ATTRIBUTES) {
        printf("\nCHYBA: Cs2Esp.dll neni vedle injectoru.\n");
        system("pause");
        return 1;
    }

    const char* arch = MachineOf(dll);
    printf("DLL arch: %s\n", arch);
    if (sizeof(void*) == 8 && strcmp(arch, "x64") != 0) {
        printf("\nCHYBA: DLL neni x64 - prebuilduj s -A x64.\n");
        system("pause");
        return 2;
    }

    SetLastError(0);
    HMODULE test = LoadLibraryW(dll);
    if (!test) {
        DWORD err = GetLastError();
        char* msg = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            nullptr, err, 0, reinterpret_cast<char*>(&msg), 0, nullptr);
        printf("\nCHYBA: DLL nelze nacist, kod %lu\n", err);
        if (msg) { printf("%s", msg); LocalFree(msg); }
        if (err == 126) printf("\nChybi zavislost (VC++ runtime nebo DLL z DrateCoreD).\n");
        if (err == 193) printf("\nSpatna architektura - prebuilduj -A x64.\n");
        system("pause");
        return 3;
    }
    FreeLibrary(test);
    printf("lokalni test zavislosti: OK\n");

    DWORD pid = FindPid(L"cs2.exe");
    if (!pid) { printf("\nCHYBA: cs2.exe nebezi.\n"); system("pause"); return 4; }
    printf("cs2.exe pid: %lu\n", pid);

    HANDLE proc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!proc) {
        printf("\nOpenProcess selhal: %lu\n", GetLastError());
        proc = OpenProcess(PROCESS_VM_WRITE | PROCESS_VM_OPERATION | PROCESS_CREATE_THREAD |
            PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!proc) { printf("Spust jako spravce.\n"); system("pause"); return 5; }
    }

    const SIZE_T bytes = (wcslen(dll) + 1) * sizeof(wchar_t);
    void* remote = VirtualAllocEx(proc, nullptr, bytes, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { printf("VirtualAllocEx: %lu\n", GetLastError()); system("pause"); return 6; }
    WriteProcessMemory(proc, remote, dll, bytes, nullptr);

    HMODULE kernel = GetModuleHandleW(L"kernel32.dll");
    auto load = reinterpret_cast<LPTHREAD_START_ROUTINE>(GetProcAddress(kernel, "LoadLibraryW"));
    HANDLE thread = CreateRemoteThread(proc, nullptr, 0, load, remote, 0, nullptr);
    if (!thread) { printf("CreateRemoteThread: %lu\n", GetLastError()); system("pause"); return 7; }

    WaitForSingleObject(thread, 10000);
    DWORD code = 0;
    GetExitCodeThread(thread, &code);
    CloseHandle(thread);

    wchar_t log[MAX_PATH]{};
    swprintf_s(log, L"%s\\cs2esp.log", exe);

    printf("\nLoadLibrary vysledek: %lu\n", code);
    printf("log existuje: %s\n", GetFileAttributesW(log) != INVALID_FILE_ATTRIBUTES ? "ANO" : "NE");

    if (code == 0) printf("\nDLL se v cilovem procesu nenacetla.\n");
    else printf("\nDLL nactena, hledej menu na Insert.\n");

    CloseHandle(proc);
    system("pause");
    return 0;
}