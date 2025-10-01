#pragma once
#include <Windows.h>
#include <cstdint>
#include <string>

int __cdecl DecryptPage(void* pageAddr);
void* __cdecl WaitForDecrypt(void* addr);
int __cdecl ReencryptPage(void* pageAddr);

void ForceDecryptAll(uintptr_t baseAddr);
void DecryptAnsi(uint8_t* dst, uint32_t len, uint32_t seed);
void DecryptWide(uint16_t* dst, uint32_t len);
std::wstring GetNameFromFName(void* entry, bool isWide, uint32_t len, uint32_t seed = 0);