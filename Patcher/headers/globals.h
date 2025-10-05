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
static constexpr uintptr_t RVA_GLogPtr = 0x74BA130;

static constexpr uintptr_t RVA_CreateVhost = 0x6F3EA10;
static constexpr uintptr_t RVA_X509Verify = 0x5D09674;
static constexpr uintptr_t RVA_ClientSSL = 0x6F4A2E0;
static constexpr uintptr_t RVA_PinCheck = 0x4CA14E0;
static constexpr uintptr_t RVA_ValidateRoot = 0x4CA11A0;
static constexpr uintptr_t RVA_UsePlatform = 0x4CA1B90;
static constexpr uintptr_t RVA_LwsLog = 0x6F3AD30;
static constexpr uintptr_t RVA_lws_ws_client_rx_sm = 0x6F49440;
static constexpr uintptr_t RVA_lws_read_h1 = 0x6F457D0;