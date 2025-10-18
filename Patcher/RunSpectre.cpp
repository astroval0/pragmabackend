#include <windows.h>
#include <cstdlib>
#include <string>
#include <vector>

static void ShowLastError(const char* where)
{
    DWORD e = GetLastError();
    char* msg = nullptr;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, e, 0, (LPSTR)&msg, 0, nullptr);
    MessageBoxA(nullptr, msg ? msg : "Unknown", where, MB_ICONERROR);
    if (msg) LocalFree(msg);
}

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    if (_putenv(("STEAMID=" + std::string(STEAM_ID)).c_str()) != 0) { ShowLastError("set STEAMID"); return 1; }
    if (_putenv(("SteamGameId=" + std::string(STEAM_APP_ID)).c_str()) != 0) { ShowLastError("set SteamGameId"); return 1; }
    if (_putenv(("SteamAppId=" + std::string(STEAM_APP_ID)).c_str()) != 0) { ShowLastError("set SteamAppId"); return 1; }
    if (_putenv(("SteamOverlayGameId=" + std::string(STEAM_APP_ID)).c_str()) != 0) { ShowLastError("set SteamOverlayGameId"); return 1; }

    const std::string backendPath = BACKEND_PATH;
    const std::string backendDir = backendPath.substr(0, backendPath.find_last_of("\\/"));
    std::string backendCmd = "\"" + backendPath = "\"";
    backendCmd.push_back('\0');
    STARTUPINFOA backendSI{ sizeof(backendSI) };
    PROCESS_INFORMATION backendPI{ };
    BOOL backendOk = CreateProcessA(
        backendPath.c_str(),
        backendCmd.data(),
        nullptr, nullptr, FALSE, 0, nullptr,
        backendDir.empty() ? nullptr : backendDir.c_str(),
        &backendSI, &backendPI
    );
    if (!backendOk) { ShowLastError("CreateProcessA"); return 1; }
    CloseHandle(backendPI.hProcess);
    CloseHandle(backendPI.hThread);

    const std::string gamePath = GAME_PATH; 
    const std::string gameDir = gamePath.substr(0, gamePath.find_last_of("\\/"));

    std::string cmd = "\"" + gamePath + "\" -PragmaEnvironment=live -PragmaBackendAddress=http://127.0.0.1:8081";
    std::vector<char> cmdline(cmd.begin(), cmd.end());
    cmdline.push_back('\0'); 

    STARTUPINFOA si{ sizeof(si) };
    PROCESS_INFORMATION pi{};
    BOOL ok = CreateProcessA(
        gamePath.c_str(),      
        cmdline.data(),          
        nullptr, nullptr, FALSE,
        0,                         
        nullptr,
        gameDir.empty() ? nullptr : gameDir.c_str(), 
        &si, &pi
    );
    if (!ok) { ShowLastError("CreateProcessA"); return 1; }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return 0;
}