#include <Windows.h>
#include <thread>
#include <iostream>

#include "headers/hooking.h"
#include "headers/consolecmd.h"
#include <filesystem>

#include "headers/lwshook.h"

uintptr_t BaseAddress = 0;

static void InitConsole()
{
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		AllocConsole();
	}

	FILE* in = nullptr; FILE* out = nullptr; FILE* err = nullptr;
	freopen_s(&in, "CONIN$", "r", stdin);
	freopen_s(&out, "CONOUT$", "w", stdout);
	freopen_s(&err, "CONOUT$", "w", stderr);

	std::ios::sync_with_stdio(false);
	std::cout.setf(std::ios::unitbuf);

	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut && hOut != INVALID_HANDLE_VALUE)
	{
		DWORD mode = 0;
		if (GetConsoleMode(hOut, &mode)) {
			SetConsoleMode(hOut, mode | ENABLE_PROCESSED_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
	}
}

static std::atomic<bool> g_Started{ false };

static DWORD WINAPI MainThreadProc(LPVOID)
{
	bool expected = false;
	if (!g_Started.compare_exchange_strong(expected, true)) return 0;

	InitConsole();

	BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
	std::cout << "[+] injected, waiting...\n";

	Sleep(1000);

	InitHooking();
	InitLWSHook();

	std::cout << "[+] all hooked launched.\n";

	std::thread(InputThread).detach();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		DisableThreadLibraryCalls(hModule);
		HANDLE h = CreateThread(nullptr, 0, MainThreadProc, nullptr, 0, nullptr);
		if (h) CloseHandle(h);
	}
	return TRUE;
}