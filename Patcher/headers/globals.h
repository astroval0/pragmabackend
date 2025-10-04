#pragma once
#include <cstdint>

typedef __int64 APlayerController;

extern uintptr_t BaseAddress;
extern APlayerController* gPlayerController;
/*
7FF7FA32B7B0 createvhost 591B7B0
7FF7FA338C20 clientssl 5928C20
7FF7F6C9DEA0 pincheck 228DEA0
7FF7F6C9B430 validateroot 228B430
7FF7F6C9C600 useplatform 228C600

7FF7F91EFC40 tickactorinput 47DFC40
7FF7F91E13D0 consolecommand 47D13D0 */

constexpr uintptr_t RVA_EngineInit = 0x1A11AE0;
constexpr auto RVA_C2Dispatcher = 0x63F0FC0;
//constexpr auto RVA_ConsoleCommand = 0x47D13D0;
//constexpr auto RVA_TickActorInput = 0x47DFC40;

static constexpr uintptr_t RVA_Serialize = 0x392F5C0;
static constexpr uintptr_t RVA_LogCmd = 0x38D4960;
static constexpr uintptr_t RVA_Logf_Internal = 0x38D3B60;
static constexpr uintptr_t RVA_GLogVerbosity = 0x74BA130;
static constexpr uintptr_t RVA_LogSuppressionMgr = 0x8F2A1C8;
static constexpr uintptr_t RVA_GLogPtr = 0x74BA130;

static constexpr uintptr_t RVA_CreateVhost = 0x591B7B0;
static constexpr uintptr_t RVA_X509Verify = 0x5D09674;
static constexpr uintptr_t RVA_ClientSSL = 0x5928C20;
static constexpr uintptr_t RVA_PinCheck = 0x228DEA0;
static constexpr uintptr_t RVA_ValidateRoot = 0x228B430;
static constexpr uintptr_t RVA_UsePlatform = 0x228C600;
static constexpr uintptr_t RVA_HttpInit = 0x4CA7E90;
static constexpr uintptr_t RVA_ByteVerifyPeer = 0x88B8D78;