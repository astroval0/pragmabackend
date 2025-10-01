#include "headers/consolecmd.h"
#include <iostream>
#include "safetyhook.hpp"
#include <cwchar>
#include <cstdlib>
#include <conio.h>

APlayerController* gPlayerController = nullptr;
SafetyHookInline ConsoleCommandHook;

FString::FString(const wchar_t* src) {
    data = _wcsdup(src);
    count = wcslen(data) + 1;
    max   = count;
}
FString::FString() { data = nullptr; count = 0; max = 0; }

FString::~FString() {
    if (data) {
        std::free(data);
        data = nullptr;
    }
    count = 0;
    max = 0;
}

FString::FString(const FString& other) {
    if (other.data) {
        data = _wcsdup(other.data);
        count = other.count;
        max = other.max;
    } else {
        data = nullptr;
        count = 0;
        max = 0;
    }
}

FString& FString::operator=(const FString& other) {
    if (this == &other) return *this;
    if (data) std::free(data);
    if (other.data) {
        data = _wcsdup(other.data);
        count = other.count;
        max = other.max;
    } else {
        data = nullptr;
        count = 0;
        max = 0;
    }
    return *this;
}

FString::FString(FString&& other) noexcept {
    data = other.data;
    count = other.count;
    max = other.max;
    other.data = nullptr;
    other.count = 0;
    other.max = 0;
}

FString& FString::operator=(FString&& other) noexcept {
    if (this == &other) return *this;
    if (data) std::free(data);
    data = other.data;
    count = other.count;
    max = other.max;
    other.data = nullptr;
    other.count = 0;
    other.max = 0;
    return *this;
}

void ExecConsoleCommand(const std::wstring& cmd) {
    if (!gPlayerController) {
        std::cout << "[!] No PlayerController yet, wait for hook." << std::endl;
        return;
    }

    empy dummy{};
    FString fcmd(cmd.c_str());

    auto ConsoleCommand =
        reinterpret_cast<__int64(*)(APlayerController*, void*, FString*, bool)>(
            BaseAddress + RVA_ConsoleCommand
        );

    ConsoleCommand(gPlayerController, &dummy, &fcmd, false);
}

inline void PrintPrompt() {
    std::cout << "\ncmd> " << std::flush;
}

void InputThread() {
    std::wstring buffer;
    PrintPrompt();

    while (true) {
        if (_kbhit()) {
            wchar_t c = _getwch();

            if (c == L'\r') {
                if (!buffer.empty()) {
                    ExecConsoleCommand(buffer);
                    buffer.clear();
                }
                PrintPrompt();
            }
            else if (c == L'\b') {
                if (!buffer.empty()) {
                    buffer.pop_back();
                    std::wcout << L"\b \b" << std::flush;
                }
            }
            else {
                buffer.push_back(c);
                std::wcout << c << std::flush;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}