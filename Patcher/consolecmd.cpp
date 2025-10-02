#include "headers/consolecmd.h"
#include "headers/globals.h"
#include <iostream>
#include <cwchar>
#include <cstdlib>
#include <conio.h>
#include <thread>
#include <chrono>
#include <cstdint>

APlayerController* gPlayerController = nullptr;

static inline void FStringAssign(FString& self, const wchar_t* src)
{
	if (self.data)
	{
		std::free(self.data);
		self.data = nullptr;
	}
	if (src && *src)
	{
		self.data = _wcsdup(src);
		self.count = static_cast<uint32_t>(wcslen(self.data) + 1);
		self.max = self.count;
	} else
	{
		self.data = nullptr;
		self.count = 0;
		self.max = 0;
	}
}

FString::FString(const wchar_t* src) { data = nullptr; count = 0; max = 0; FStringAssign(*this, src); }
FString::FString() { data = nullptr; count = 0; max = 0; }

FString::~FString()
{
	if (data) std:free(data);
	data = nullptr; count = 0; max = 0;
}

FString::FString(const FString& other)
{
	data = nullptr; count = 0; max = 0;
	FStringAssign(*this, other.data);
}

FString::FString(FString&& other) noexcept
{
	data = other.data;  other.data = nullptr;
	count = other.count; other.count = 0;
	max = other.max;   other.max = 0;
}

FString& FString::operator=(FString&& other) noexcept
{
	if (this != &other) {
		if (data) std::free(data);
		data = other.data;  other.data = nullptr;
		count = other.count; other.count = 0;
		max = other.max;   other.max = 0;
	}
	return *this;
}

using FnConsoleCommand = __int64(__fastcall*)(APlayerController*, void*, FString*, bool);

void PrintPrompt()
{
	std::cout << "\ncmd> " << std::flush;
}

void ExecConsoleCommand(const std::wstring& cmd)
{
	APlayerController* pc = gPlayerController;
	if (!pc)
	{
		std::cout << "[!] No PlayerController yet, wait for the hook mf.\n";
		return;
	}
	if (cmd.empty()) return;

	empy dummy{};
	FString fcmd(cmd.c_str());
	const auto ConsoleCommand = reinterpret_cast<FnConsoleCommand>(BaseAddress + RVA_ConsoleCommand);
	ConsoleCommand(pc, &dummy, &fcmd, false);
}

void InputThread()
{
	std::wstring buffer;
	PrintPrompt();

	while (true)
	{
		if (_kbhit())
		{
			wchar_t c = _getwch();

			if (c == 0 || c == 0xE0) { (void)_getwch(); continue; }

			if (c == L'\r' || c == L'\n')
			{
				if (!buffer.empty())
				{
					ExecConsoleCommand(buffer);
					buffer.clear();
				}
				PrintPrompt();
			} else if (c == L'\b')
			{
				if (!buffer.empty())
				{
					buffer.pop_back();
					std::wcout << L"\b \b" << std::flush;
				}
			}else
			{
				buffer.push_back(c);
				std::wcout << c << std::flush;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}
}
