#include "headers/hooking.h"
#include "headers/decrypt.h"
#include "headers/consolecmd.h"
#include <iostream>

static SafetyHookInline TickHook;
static bool pcCaptured = false;

constexpr auto RVA_TickActorInput = 0x5B51070; //todo move ts to a header
__int64 __fastcall TickHookFn(APlayerController* pc, void* unused, float deltaSeconds) {
    // lightweight keepalive
    DecryptPage(reinterpret_cast<void *>(BaseAddress + RVA_TickActorInput));
    DecryptPage(reinterpret_cast<void *>(BaseAddress + 0x5B500B0)); //todo use header var

    static int counter = 0;
    if (++counter >= 600) {
        ForceDecryptAll(BaseAddress);
        counter = 0;
    }

    if (!pcCaptured) {
        gPlayerController = pc;
        pcCaptured = true;
        std::cout<< "[+] captured APlayerController @ " << std::hex << pc << std::endl;
        PrintPrompt();
    }
    return TickHook.call<__int64>(pc, unused, deltaSeconds);
}

void InitHooking() {
    BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleA("SpectreClient-Win64-Shipping.exe"));
    ForceDecryptAll(BaseAddress);

    void* tickTarget = reinterpret_cast<void *>(BaseAddress + RVA_TickActorInput);
    TickHook = safetyhook::create_inline(reinterpret_cast<uintptr_t>(tickTarget), TickHookFn);

    std::cout << "[+] Hooks installed." << std::endl;
}