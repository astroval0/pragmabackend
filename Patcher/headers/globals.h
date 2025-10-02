#pragma once
#include <cstdint>

typedef __int64 APlayerController;

extern uintptr_t BaseAddress;
extern APlayerController* gPlayerController;

constexpr uintptr_t RVA_EngineInit = 0x1A11AE0;
constexpr auto RVA_C2Dispatcher = 0x63F0FC0;
constexpr auto RVA_ConsoleCommand = 0x5B500B0;
constexpr auto RVA_TickActorInput = 0x5B51070;

static constexpr uintptr_t RVA_Serialize = 0x392F5C0;
static constexpr uintptr_t RVA_LogCmd = 0x38D4960;
static constexpr uintptr_t RVA_Logf_Internal = 0x38D3B60;
static constexpr uintptr_t RVA_GLogVerbosity = 0x74BA130;
static constexpr uintptr_t RVA_LogSuppressionMgr = 0x8F2A1C8;
static uintptr_t RVA_GLogPtr = 0x74BA130;


