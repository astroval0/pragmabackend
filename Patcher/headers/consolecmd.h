#pragma once
#include <string>
#include "globals.h"


struct FString {
    wchar_t* data;
    unsigned int count;
    unsigned int max;
    explicit FString(const wchar_t* src);
    FString();
    ~FString();
    FString(const FString& other);
    FString& operator=(const FString& other);
    FString(FString&& other) noexcept;
    FString& operator=(FString&& other) noexcept;
};

struct empy { unsigned char x[0x100]; };

void ExecConsoleCommand(const std::wstring& cmd);
void InputThread();
void PrintPrompt();

__int64 __fastcall ConsoleCommand_Hook(APlayerController* pc, void* unused, class FString* cmd, bool bWriteToLog);