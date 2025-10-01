#include <Windows.h>
#include <thread>
#include <iostream>

#include "headers/hooking.h"
#include "headers/consolecmd.h"
#include <filesystem>

#include "headers/lwshook.h"

uintptr_t BaseAddress = 0;


static void InitConsole() {
    AllocConsole();
    FILE* in = nullptr; FILE* out = nullptr; FILE* err = nullptr;
    freopen_s(&in, "CONIN$", "r", stdin);
    freopen_s(&out, "CONOUT$", "w", stdout);
    freopen_s(&err, "CONOUT$", "w", stderr);
    std::ios::sync_with_stdio(false);
    std::cout.setf(std::ios::unitbuf);
}

static void MainThread() {
    InitConsole();
    std::cout << "[+] injected, waiting..." << std::endl;
    Sleep(2000);

    InitHooking();
    InitLWSHook();

    std::cout << "[+] all hooks launched." << std::endl;
    std::thread(InputThread).detach();
}

BOOL APIENTRY DllMain(HMODULE hmodule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmodule);
        std::thread(MainThread).detach();
    }
    return TRUE;
}