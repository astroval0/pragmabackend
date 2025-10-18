#include <Windows.h>
#include <atomic>
#include "headers/lwshook.h"

uintptr_t BaseAddress = 0;

static std::atomic<bool> g_Started{ false };

static DWORD WINAPI MainThreadProc(LPVOID) {
    bool expected = false;
    if (!g_Started.compare_exchange_strong(expected, true)) return 0;

    BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    InitLWSHook();
    return 0;
}
// raaaaaaaaa this better work
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE h = CreateThread(nullptr, 0, MainThreadProc, nullptr, 0, nullptr);
        if (h) CloseHandle(h);
    }
    return TRUE;
}