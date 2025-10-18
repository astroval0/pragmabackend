#pragma once
#include <cstdint>

typedef __int64 APlayerController;

extern uintptr_t BaseAddress;
extern APlayerController* gPlayerController;

// offsets for feb 25 2025 SD build (171268)

constexpr auto RVA_ConsoleCommand = 0x5B57B70;
constexpr auto RVA_TickActorInput = 0x5B58B30;

static constexpr uintptr_t RVA_CreateVhost = 0x6F49BB0;
static constexpr uintptr_t RVA_X509Verify = 0x5D11334;
static constexpr uintptr_t RVA_ClientSSL = 0x6F55480;
static constexpr uintptr_t RVA_PinCheck = 0x4CA4F28;
static constexpr uintptr_t RVA_ValidateRoot = 0x4CA4BF0;
static constexpr uintptr_t RVA_UsePlatform = 0x4CA55E0;
static constexpr uintptr_t RVA_LwsLog = 0x6F45ED0;
static constexpr uintptr_t RVA_lws_ws_client_rx_sm = 0x6F545D6;
static constexpr uintptr_t RVA_lws_read_h1 = 0x6F50970;
static constexpr uintptr_t RVA_LwsParseFrame = 0x6F580E0;
static constexpr uintptr_t RVA_Gamesight = 0x3340C30;
static constexpr uintptr_t RVA_FmtW = 0x3835190;
static constexpr uintptr_t RVA_VfmtPtr = 0x705AAF8;