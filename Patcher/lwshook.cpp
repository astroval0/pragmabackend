#include "headers/lwshook.h"
#include "safetyhook.hpp"
#include <iostream>
#include <windows.h>
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <string>
#include <filesystem>

#include "headers/decrypt.h"
#include "headers/globals.h"

static SafetyHookInline g_LwsVhostHook;
static SafetyHookInline g_PinCheckHook;
static SafetyHookInline g_ClientSSLHook;
static SafetyHookInline g_X509VerifyHook;
static SafetyHookInline g_ValidateRootHook;
static SafetyHookInline g_UsePlatformCertsHook;
static SafetyHookInline g_LwsLogHook;
static SafetyHookInline g_LwsLogFormatHook;
static SafetyHookInline g_RxSMHook;
static SafetyHookInline g_ReadH1Hook;

using FnCreateVhost			= __int64(__fastcall*)(__int64, int*);
using FnPinCheck				= __int64(__fastcall*)(__int64, __int64);
using FnClientSSLInit			= int(__fastcall*)(int* a2, __int64 vhost);
using FnSSL_CTX_set_verify	= void(__fastcall*)(void* ctx, int mode, void* callback);
using FnValidateRoot			= __int64(__fastcall*)(__int64 a1, __int64 a2);
using FnUsePlatformCerts	= __int64(__fastcall*)(__int64 a1);
using FnLwsLog = __int64(__fastcall*)(int32_t level, const char* fmt, const void* args);
using FnLwsLogFormat = __int64(__fastcall*)(int32_t level, const char* fmt, void* va_args);

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
static __int64 __fastcall hk_LwsLog(int32_t level, const char* fmt, const void* args)
{
	std::cout << std::hex
		<< "[dbg] level=" << level
		<< " fmt=" << (void*)fmt
		<< " args=" << (void*)args
		<< std::dec << "\n";

	if (IsReadable(args, 32)) {
		const unsigned char* p = (const unsigned char*)args;
		std::cout << "[dbg] arg bytes:";
		for (int i = 0; i < 32; i++) std::cout << " " << std::hex << (int)p[i];
		std::cout << std::dec << "\n";
	}

	return g_LwsLogHook ? g_LwsLogHook.call<__int64>(level, fmt, args) : 0;
}
static bool LooksLikeString(const char* p)
{
	if (!IsReadable(p, 1)) return false;
	for (int i = 0; i < 256; i++) {
		unsigned char c;
		if (!IsReadable(p + i, 1)) return false;
		c = (unsigned char)p[i];
		if (c == 0) return i > 0;     
		if (c < 0x09 || c > 0x7E) return false; 
	}
	return false;
}
static __int64 __fastcall hk_LwsLogFormat(int32_t level,const char* fmt,const char* maybe_text)
{
	const char* text = nullptr;
	char safe[128] = "<non-text log call>";

	if (LooksLikeString(maybe_text))
		text = maybe_text;
	else if (LooksLikeString(fmt))
		text = fmt;
	else
		text = safe;

	char exe[MAX_PATH];
	GetModuleFileNameA(nullptr, exe, MAX_PATH);
	char* slash = strrchr(exe, '\\');
	if (slash) *slash = '\0';
	strcat_s(exe, "\\lws_logs.txt");

	FILE* f = nullptr;
	fopen_s(&f, exe, "a");
	if (f) {
		fprintf(f, "[lvl %d] %s\n", level, text);
		fclose(f);
	}

	std::cout << "[lvl " << level << "] " << text << "\n";

	return g_LwsLogFormatHook
		? g_LwsLogFormatHook.call<__int64>(level, fmt, maybe_text)
		: 0;
}
static int64_t __fastcall hk_lws_ws_client_rx_sm(void* arg1, int64_t* arg2, int64_t arg3, void* arg4)
{
	auto ret = g_RxSMHook.call<int64_t>(arg1, arg2, arg3, arg4);
	uint32_t state = *(uint32_t*)((char*)arg1 + 0x70);
	uint32_t flags = state >> 16;
	uint32_t code = state & 0xFFFF;
	uint32_t close_reason = 0;
	uint8_t timeout_flag = 0;

	if (IsReadable((char*)arg1 + 0x1BC, 4))
		close_reason = *(uint32_t*)((char*)arg1 + 0x1BC);

	if (IsReadable((char*)arg1 + 0x1C7, 1))
		timeout_flag = *(uint8_t*)((char*)arg1 + 0x1C7);

	std::cout << std::hex
		<< "[lws] ws_client_rx_sm ret=" << ret
		<< " state=" << code
		<< " flags=" << flags
		<< " close_reason=" << close_reason
		<< " timeout=" << (int)timeout_flag
		<< std::dec << "\n";

	if (ret)
		std::cout << "[lws] ws_client_rx_sm error ret=" << std::hex << ret
		<< " full_state=" << state
		<< " close_reason=" << close_reason
		<< std::dec << "\n";

	return ret;
}
static int64_t __fastcall hk_lws_read_h1(void* arg1, int64_t arg2, int64_t arg3, void* arg4)
{
	auto ret = g_ReadH1Hook.call<int64_t>(arg1, arg2, arg3, arg4);
	uint32_t state = 0;
	uint32_t close_reason = 0;
	uint8_t timeout_flag = 0;

	if (IsReadable((char*)arg1 + 0x70, 4))
		state = *(uint32_t*)((char*)arg1 + 0x70);

	if (IsReadable((char*)arg1 + 0x1BC, 4))
		close_reason = *(uint32_t*)((char*)arg1 + 0x1BC);

	if (IsReadable((char*)arg1 + 0x1C7, 1))
		timeout_flag = *(uint8_t*)((char*)arg1 + 0x1C7);

	if (ret == 0xFFFFFFFF)
		std::cout << "[lws] lws_read_h1 fatal drop state=" << std::hex << state
		<< " close_reason=" << close_reason
		<< " timeout=" << (int)timeout_flag
		<< std::dec << "\n";

	return ret;
}

void InitX509VerifyHook() { InstallInlineHook(g_X509VerifyHook, RVA_X509Verify, &hk_X509_verify_cert, "X509VerifyCert"); }
void InitValidateRootHook() { InstallInlineHook(g_ValidateRootHook, RVA_ValidateRoot, &hk_ValidateRootCerts, "ValidateRootCerts"); }
void InitUsePlatformHook() { InstallInlineHook(g_UsePlatformCertsHook, RVA_UsePlatform, &hk_UsePlatformCerts, "UsePlatformCerts"); }
void InitPinCheckHook() { InstallInlineHook(g_PinCheckHook, RVA_PinCheck, &hk_PinCheck, "PinCheck"); }
void InitClientSSLHook() { InstallInlineHook(g_ClientSSLHook, RVA_ClientSSL, &hk_ClientSSLInit, "ClientSSLInit"); }
void InitLwsLoggingHook()
{
	InstallInlineHook(g_LwsLogHook, RVA_LwsLog, &hk_LwsLog, "LwsLog");
	InstallInlineHook(g_LwsLogFormatHook, RVA_LwsLog + 0x30, &hk_LwsLogFormat, "LwsLogFormat");
	InstallInlineHook(g_RxSMHook, RVA_lws_ws_client_rx_sm, &hk_lws_ws_client_rx_sm, "lws_ws_client_rx_sm");
	InstallInlineHook(g_ReadH1Hook, RVA_lws_read_h1, &hk_lws_read_h1, "lws_read_h1");
}

void InitLWSHook()
{
	if (!BaseAddress) { BaseAddress = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }

	InitX509VerifyHook();
	InitPinCheckHook();
	InitClientSSLHook();
	InitValidateRootHook();
	InitUsePlatformHook();
	InitLwsLoggingHook();

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