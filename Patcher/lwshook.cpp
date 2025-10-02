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

using FnCreateVhost			= __int64(__fastcall*)(__int64, int*);
using FnPinCheck				= __int64(__fastcall*)(__int64, __int64);
using FnClientSSLInit			= int(__fastcall*)(int* a2, __int64 vhost);
using FnSSL_CTX_set_verify	= void(__fastcall*)(void* ctx, int mode, void* callback);
using FnValidateRoot			= __int64(__fastcall*)(__int64 a1, __int64 a2);
using FnUsePlatformCerts	= __int64(__fastcall*)(__int64 a1);

constexpr size_t OFFSET_WhitelistByte = 0x68;
constexpr size_t OFFSET_HostPtr = 0x140;
constexpr size_t OFFSET_HostLen = 0x148;

static bool IsReadable(const void* addr, size_t size)
{
	if (!addr || size == 0) return false;
	const auto* p = static_cast<const unsigned char*>(addr);
	const auto* end = p + size;

	while (p < end)
	{
		MEMORY_BASIC_INFORMATION mbi{};
		if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
		if (mbi.State != MEM_COMMIT) return false;
		if (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS)) return false;

		const DWORD prot = (mbi.Protect & 0xFF);
		const bool readable =
			prot == PAGE_READONLY ||
			prot == PAGE_READWRITE ||
			prot == PAGE_WRITECOPY ||
			prot == PAGE_EXECUTE_READ ||
			prot == PAGE_EXECUTE_READWRITE ||
			prot == PAGE_EXECUTE_WRITECOPY;
		if (!readable) return false;

		const auto* next = static_cast<const unsigned char*>(mbi.BaseAddress) + mbi.RegionSize;
		if (next <= p) return false;

		p = next;
	}
	return true;
}

template<typename HookFn>
static void InstallInlineHook(SafetyHookInline& outHook, uintptr_t rva, HookFn* hookFn, const char* tag)
{
	const uintptr_t target = BaseAddress + rva;
	DecryptPage(reinterpret_cast<void*>(target));
	try
	{
		outHook = safetyhook::create_inline(reinterpret_cast<void*>(target), reinterpret_cast<void*>(hookFn));
		std::cout << "[lws] " << tag << " installed @ 0x" << std::hex << target << std::dec << "\n";
	} catch (...)
	{
		std::cout << "[lws] failed " << tag << "\n";
	}
}

static int __fastcall hk_X509_verify_cert(void*) //ctx
{
	std::cout << "[lws] X509_verify_cert -> 1\n";
	return 1;
}
static __int64 __fastcall hk_ValidateRootCerts(__int64, __int64) // a1, a2
{
	std::cout << "[lws] ValidateRootCerts -> 0\n";
	return 0;
}
static __int64 __fastcall hk_UsePlatformCerts(__int64) // a1
{
	std::cout << "[lws] UsePlatformCerts -> 0\n";
	return 0;
}
static __int64 __fastcall hk_PinCheck(__int64, __int64) // a1, a2
{
	std::cout << "[lws] PinCheck -> 0";
	return 0;
}
static int __fastcall hk_ClientSSLInit(int* a2, __int64 vhost)
{
	std::cout << "[lws] ClientSSLInit hook fired\n";
	return g_ClientSSLHook ? g_ClientSSLHook.call<int>(a2, vhost) : 0;
}
static __int64 __fastcall hk_CreateVhost(__int64 a1, int* a2)
{
	thread_local bool inHook = false;

	if (inHook) return g_LwsVhostHook ? g_LwsVhostHook.call<__int64>(a1, a2) : 0;

	struct ReentryGuard
	{
		bool& flag; bool armed{ false };
		explicit ReentryGuard(bool& f) : flag(f)
		{
			if (!flag)
			{
				flag = true;
				armed = true;
			}
		}
		~ReentryGuard() { if (armed)flag = false; }
	} guard(inHook);

	std::cout << "[lws] vhost hook fired, a1=0x" << std::hex << a1<< " a2=0x" << reinterpret_cast<uintptr_t>(a2) << std::dec << "\n";
	if (a2)
	{
		const uintptr_t base = reinterpret_cast<uintptr_t>(a2);
		auto* pWhitelist = reinterpret_cast<uint8_t*>(base + OFFSET_WhitelistByte);
		if (IsReadable(pWhitelist, sizeof(uint8_t)) && *pWhitelist == 0)
		{
			*pWhitelist = 1;
			std::cout << "[lws] DisableDomainWhitelist -> 1\n";
		}

		auto* pHostWPtr = reinterpret_cast<const wchar_t**>(base + OFFSET_HostPtr);
		auto* pHostWLen = reinterpret_cast<uint32_t*>(base + OFFSET_HostLen);
		static std::atomic<bool> logged{ false };

		if (IsReadable(pHostWPtr, sizeof(void*)) && IsReadable(pHostWLen, sizeof(uint32_t)))
		{
			const wchar_t* hostW = *pHostWPtr;
			const uint32_t hostLen = *pHostWLen;

			if (hostW && hostLen < (1u << 16))
			{
				if (IsReadable(hostW, static_cast<size_t>(hostLen) * sizeof(wchar_t)))
				{
					if (!logged.exchange(true))
					{
						std::wcout << L"[lws] hostW seen: len=" << hostLen << L" str=" << std::wstring(hostW, hostLen) << L"\n";
					}
				}
			}
		}
	} return g_LwsVhostHook ? g_LwsVhostHook.call<__int64>(a1, a2) : 0;
} 

void InitX509VerifyHook() { InstallInlineHook(g_X509VerifyHook, RVA_X509Verify, &hk_X509_verify_cert, "X509VerifyCert"); }
void InitValidateRootHook() { InstallInlineHook(g_ValidateRootHook, RVA_ValidateRoot, &hk_ValidateRootCerts, "ValidateRootCerts"); }
void InitUsePlatformHook() { InstallInlineHook(g_UsePlatformCertsHook, RVA_UsePlatform, &hk_UsePlatformCerts, "UsePlatformCerts"); }
void InitPinCheckHook() { InstallInlineHook(g_PinCheckHook, RVA_PinCheck, &hk_PinCheck, "PinCheck"); }
void InitClientSSLHook() { InstallInlineHook(g_ClientSSLHook, RVA_ClientSSL, &hk_ClientSSLInit, "ClientSSLInit"); }

void InitLWSHook()
{
	if (!BaseAddress) { BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

	InitX509VerifyHook();
	InitPinCheckHook();
	InitClientSSLHook();
	InitValidateRootHook();
	InitUsePlatformHook();

	const uintptr_t targetVhost = BaseAddress + RVA_CreateVhost;
	DecryptPage(reinterpret_cast<void*>(targetVhost));
	try
	{
		g_LwsVhostHook = safetyhook::create_inline(reinterpret_cast<void*>(targetVhost),reinterpret_cast<void*>(&hk_CreateVhost));
		std::cout << "[+] base addr 0x" << std::hex << BaseAddress << std::dec << "\n";
		std::cout << "[lws] vhost installed @ 0x" << std::hex << targetVhost << std::dec << "\n";
	} catch (...)
	{
		std::cout << "[lws] failed vhost\n";
	}
}