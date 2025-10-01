#include "headers/lwshook.h"
#include "safetyhook.hpp"
#include <iostream>
#include <windows.h>
#include <atomic>

#include "headers/decrypt.h"
#include "headers/globals.h"

static SafetyHookInline g_LwsVhostHook;
static SafetyHookInline g_PinCheckHook;
static SafetyHookInline g_ClientSSLHook;
static SafetyHookInline g_X509VerifyHook;
static SafetyHookInline g_ValidateRootHook;
static SafetyHookInline g_UsePlatformCertsHook;

static constexpr uintptr_t RVA_CreateVhost    = 0x6F3EA10;
static constexpr uintptr_t RVA_X509Verify     = 0x5D09674;
static constexpr uintptr_t RVA_ClientSSL      = 0x6F4A2E0;
static constexpr uintptr_t RVA_PinCheck       = 0x4CA14E0;
static constexpr uintptr_t RVA_ValidateRoot   = 0x4CA11A0;
static constexpr uintptr_t RVA_UsePlatform    = 0x4CA1B90;

using FnCreateVhost        = __int64(__fastcall*)(__int64, int*);
using FnPinCheck           = __int64(__fastcall*)(__int64, __int64);
using FnClientSSLInit      = int(__fastcall*)(int* a2, __int64 vhost);
using FnSSL_CTX_set_verify = void(__fastcall*)(void* ctx, int mode, void* callback);
using FnValidateRoot       = __int64(__fastcall*)(__int64 a1, __int64 a2);
using FnUsePlatformCerts   = __int64(__fastcall*)(__int64 a1);

static bool IsReadable(const void* addr, size_t size) {
    if (!addr || size == 0) return false;
    const unsigned char* p = static_cast<const unsigned char*>(addr);
    const unsigned char* end = p + size;
    while (p < end) {
        MEMORY_BASIC_INFORMATION mbi{};
        if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
        if (mbi.State != MEM_COMMIT) return false;
        if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;
        const DWORD prot = mbi.Protect & 0xff;
        const bool readable =
            prot == PAGE_READONLY || prot == PAGE_READWRITE || prot == PAGE_WRITECOPY ||
            prot == PAGE_EXECUTE_READ || prot == PAGE_EXECUTE_READWRITE || prot == PAGE_EXECUTE_WRITECOPY;
        if (!readable) return false;
        unsigned char* next = static_cast<unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
        if (next <= p) return false;
        p = next;
    }
    return true;
}

static int __fastcall hk_X509_verify_cert(void* ctx) {
    std::cout << "[lws] X509_verify_cert bypassed (forcing success)\n";
    return 1;
}

void InitX509VerifyHook() {
    uintptr_t target = BaseAddress + RVA_X509Verify;
    DecryptPage(reinterpret_cast<void*>(target));
    try {
        g_X509VerifyHook = safetyhook::create_inline(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(&hk_X509_verify_cert));
        std::cout << "[lws] X509_verify_cert hook installed @ 0x"
                  << std::hex << target << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install X509_verify_cert hook\n";
    }
}

static __int64 __fastcall hk_ValidateRootCerts(__int64 a1, __int64 a2) {
    std::cout << "[lws] bypass ValidateRootCertificates\n";
    return 0;
}

void InitValidateRootHook() {
    uintptr_t target = BaseAddress + RVA_ValidateRoot;
    DecryptPage(reinterpret_cast<void*>(target));
    try {
        g_ValidateRootHook = safetyhook::create_inline(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(&hk_ValidateRootCerts));
        std::cout << "[lws] ValidateRootCertificates hook installed @ 0x"
                  << std::hex << target << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install ValidateRootCertificates hook\n";
    }
}

static __int64 __fastcall hk_UsePlatformCerts(__int64 a1) {
    std::cout << "[lws] bypass UsePlatformProvidedCertificates\n";
    return 0;
}

void InitUsePlatformHook() {
    uintptr_t target = BaseAddress + RVA_UsePlatform;
    DecryptPage(reinterpret_cast<void*>(target));
    try {
        g_UsePlatformCertsHook = safetyhook::create_inline(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(&hk_UsePlatformCerts));
        std::cout << "[lws] UsePlatformProvidedCertificates hook installed @ 0x"
                  << std::hex << target << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install UsePlatformProvidedCertificates hook\n";
    }
}

static __int64 __fastcall hk_CreateVhost(__int64 a1, int* a2) {
    thread_local bool inHook = false;
    if (inHook) {
        if (g_LwsVhostHook) return g_LwsVhostHook.call<__int64>(a1, a2);
        return 0;
    }
    inHook = true;

    std::cout << "[lws] vhost hook fired, a1=0x" << std::hex << a1
              << " a2=0x" << reinterpret_cast<uintptr_t>(a2) << std::dec << "\n";

    if (a2) {
        uintptr_t base = reinterpret_cast<uintptr_t>(a2);
        auto pWhitelist = reinterpret_cast<uint8_t*>(base + 0x68);
        if (IsReadable(pWhitelist, sizeof(uint8_t)) && *pWhitelist == 0) {
            *pWhitelist = 1;
            std::cout << "[lws] disable_domain_whitelist -> 1\n";
        }

        auto pHostWPtr = reinterpret_cast<const wchar_t**>(base + 0x140);
        auto pHostWLen = reinterpret_cast<uint32_t*>(base + 0x148);
        static std::atomic<bool> logged{false};
        if (IsReadable(pHostWPtr, sizeof(void*)) && IsReadable(pHostWLen, sizeof(uint32_t))) {
            const wchar_t* hostW = *pHostWPtr;
            uint32_t hostLen = *pHostWLen;
            if (hostW && hostLen && !logged.exchange(true)) {
                std::wcout << L"[lws] hostW seen: len=" << hostLen
                           << L" str=" << std::wstring(hostW, hostLen) << L"\n";
            }
        }
    }

    auto ret = g_LwsVhostHook ? g_LwsVhostHook.call<__int64>(a1, a2) : 0;
    inHook = false;
    return ret;
}

static __int64 __fastcall hk_PinCheck(__int64 a1, __int64 a2) {
    std::cout << "[lws] pubkey pin check bypassed\n";
    return 0;
}

void InitPinCheckHook() {
    uintptr_t targetPin = BaseAddress + RVA_PinCheck;
    DecryptPage(reinterpret_cast<void*>(targetPin));
    try {
        g_PinCheckHook = safetyhook::create_inline(
            reinterpret_cast<void*>(targetPin),
            reinterpret_cast<void*>(&hk_PinCheck));
        std::cout << "[lws] pubkey pin bypass hook installed @ 0x"
                  << std::hex << targetPin << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install pubkey pin hook\n";
    }
}

static int __fastcall hk_ClientSSLInit(int* a2, __int64 vhost) {
    std::cout << "[lws] client ssl init hook fired\n";
    if (g_ClientSSLHook) {
        return g_ClientSSLHook.call<int>(a2, vhost);
    }
    return 0;
}

void InitClientSSLHook() {
    uintptr_t target = BaseAddress + RVA_ClientSSL;
    DecryptPage(reinterpret_cast<void*>(target));
    try {
        g_ClientSSLHook = safetyhook::create_inline(
            reinterpret_cast<void*>(target),
            reinterpret_cast<void*>(&hk_ClientSSLInit));
        std::cout << "[lws] client ssl init hook installed @ 0x"
                  << std::hex << target << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install client ssl init hook\n";
    }
}

void InitLWSHook() {
    if (!BaseAddress) {
        BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    }

    InitX509VerifyHook();
    InitPinCheckHook();
    InitClientSSLHook();
    InitValidateRootHook();
    InitUsePlatformHook();

    uintptr_t targetVhost = BaseAddress + RVA_CreateVhost;
    DecryptPage(reinterpret_cast<void*>(targetVhost));
    try {
        g_LwsVhostHook = safetyhook::create_inline(
            reinterpret_cast<void*>(targetVhost),
            reinterpret_cast<void*>(&hk_CreateVhost));
        std::cout << "[+] base addr 0x" << std::hex << BaseAddress << std::dec << "\n";
        std::cout << "[lws] vhost hook installed @ 0x"
                  << std::hex << targetVhost << std::dec << "\n";
    } catch (...) {
        std::cout << "[lws] failed to install vhost hook\n";
    }
}