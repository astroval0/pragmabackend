#include <windows.h>
#include <cstdlib>
#include <string>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    // Set environment variables
    if (_putenv((std::string("STEAMID=") + STEAM_ID).c_str()) != 0) {
        MessageBoxA(nullptr, "Failed to set STEAMID", "Environment Error", MB_ICONERROR);
        return 1;
    }

    if (_putenv((std::string("SteamGameId=") + STEAM_APP_ID).c_str()) != 0) {
        MessageBoxA(nullptr, "Failed to set SteamGameId", "Environment Error", MB_ICONERROR);
        return 1;
    }

    if (_putenv((std::string("SteamAppId=") + STEAM_APP_ID).c_str()) != 0) {
        MessageBoxA(nullptr, "Failed to set SteamAppId", "Environment Error", MB_ICONERROR);
        return 1;
    }

    if (_putenv((std::string("SteamOverlayGameId=") + STEAM_APP_ID).c_str()) != 0) {
        MessageBoxA(nullptr, "Failed to set SteamOverlayGameId", "Environment Error", MB_ICONERROR);
        return 1;
    }

    // Build command line
    std::string launchBackendCommand = std::string("\"") + BACKEND_PATH + "\"";

    // Set up process startup info
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL backendsuccess = CreateProcessA(
        nullptr,
        launchBackendCommand.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        BACKEND_DIR,
        &si,
        &pi
    );
    std::string launchSpectreCommand = std::string("\"") + GAME_PATH + "\" -PragmaEnvironment=live -log -nosteam -PragmaBackendAddress=https://game.astro-dev.uk";

    // Launch the game
    BOOL success = CreateProcessA(
        nullptr,                // Application name
        launchSpectreCommand.data(),         // Command line
        nullptr,                // Process security
        nullptr,                // Thread security
        FALSE,                  // Inherit handles
        CREATE_NO_WINDOW,       // No console window
        nullptr,                // Inherit environment
        nullptr,                // Current directory
        &si,                    // Startup info
        &pi                     // Process info
    );

    if (!success) {
        MessageBoxA(nullptr, "Failed to launch the game.", "Error", MB_ICONERROR);
        return 1;
    }

    // Optionally wait for the game to exit
    // WaitForSingleObject(pi.hProcess, INFINITE);

    // Clean up handles
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return 0;
}