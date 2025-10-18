#include <windows.h>
#include <fstream>
#include <string>
#include <chrono>
#include <iomanip>

void Log(const std::string& message) {
    std::ofstream logfile("be_proxy.log", std::ios_base::app);
    if (logfile.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto in_time_t = std::chrono::system_clock::to_time_t(now);
        logfile << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X") << " - " << message << std::endl;
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        Log("Fake BEClient_x64.dll attached to process.");
    }
    return TRUE;
}

extern "C" {
    __declspec(dllexport) bool __fastcall Init(__int64 a1, void* a2, void* a3) {
        std::string log_message = "BEClient_Init() called with args: a1=" + std::to_string(a1) +
            ", a2=" + std::to_string(reinterpret_cast<uintptr_t>(a2)) +
            ", a3=" + std::to_string(reinterpret_cast<uintptr_t>(a3)) +
            ". Returning TRUE.";
        Log(log_message);
        return true;
    }

    __declspec(dllexport) void __stdcall Run() {}

    __declspec(dllexport) void __stdcall Shutdown() {
        Log("BEClient_Shutdown() called. Acknowledged.");
    }
}