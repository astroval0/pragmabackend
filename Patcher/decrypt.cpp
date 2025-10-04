#include "headers/decrypt.h"
#include <string>
#include <cwchar>

// theia page decryption
BOOL __stdcall DecryptPage(void* pageAddr) {
    return TRUE;
}

BOOL __stdcall ReencryptPage(void* pageAddr) {
    DWORD oldProtect = 0;
    if (VirtualProtect(pageAddr, 0x1000, PAGE_NOACCESS, &oldProtect)) {
        __try {
            volatile uint8_t tmp = *reinterpret_cast<uint8_t*>(pageAddr);
            (void)tmp;
        } __except (EXCEPTION_EXECUTE_HANDLER) { }
        VirtualProtect(pageAddr, 0x1000, oldProtect, &oldProtect);
        return TRUE;
    }
    return FALSE;
}

void* WaitForDecrypt(void* addr) {
    MEMORY_BASIC_INFORMATION mbi{};
    for (;;) {
        if (VirtualQuery(addr, &mbi, sizeof(mbi)) == 0)
            return nullptr;

        if (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) {
            return addr;
        }

        DecryptPage(addr);
        Sleep(5);
    }
}

// step .text
void ForceDecryptAll(uintptr_t baseAddr) {
    auto dos = reinterpret_cast<PIMAGE_DOS_HEADER>(baseAddr);
    auto nt  = reinterpret_cast<PIMAGE_NT_HEADERS>(baseAddr + dos->e_lfanew);

    auto sections = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        const auto& sec = sections[i];
        if (strncmp((char*)sec.Name, ".text", 5) == 0) {
            BYTE* start = reinterpret_cast<BYTE *>(baseAddr + sec.VirtualAddress);
            size_t size = sec.Misc.VirtualSize;
            for (size_t offset = 0; offset < size; offset += 0x1000) {
                DecryptPage(start + offset);
            }
        }
    }
}