#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <Unknwn.h>
#include <filesystem>
#include <string>

using PFN_CreateDXGIFactory = HRESULT(WINAPI*)(REFIID, void**);
using PFN_CreateDXGIFactory1 = HRESULT(WINAPI*)(REFIID, void**);
using PFN_CreateDXGIFactory2 = HRESULT(WINAPI*)(UINT, REFIID, void**);
using PFN_DXGIDeclareAdapterRemovalSupport = HRESULT(WINAPI*)();
using PFN_DXGIGetDebugInterface = HRESULT(WINAPI*)(REFIID, void**);
using PFN_DXGIGetDebugInterface1 = HRESULT(WINAPI*)(UINT, REFIID, void**);

static HMODULE g_real = nullptr;
static PFN_CreateDXGIFactory          pCreateDXGIFactory = nullptr;
static PFN_CreateDXGIFactory1         pCreateDXGIFactory1 = nullptr;
static PFN_CreateDXGIFactory2         pCreateDXGIFactory2 = nullptr;
static PFN_DXGIDeclareAdapterRemovalSupport pDXGIDeclareAdapterRemovalSupport = nullptr;
static PFN_DXGIGetDebugInterface      pDXGIGetDebugInterface = nullptr;
static PFN_DXGIGetDebugInterface1     pDXGIGetDebugInterface1 = nullptr;

static std::wstring GetSelfDir(const HMODULE self) {
    wchar_t me[MAX_PATH]{}; GetModuleFileNameW(self, me, MAX_PATH);
    const std::filesystem::path p(me);
    return p.parent_path().native();
}

static void LoadRealDxgi() {
    if (g_real) return;
    wchar_t sysdir[MAX_PATH]{};
    if (!GetSystemDirectoryW(sysdir, MAX_PATH)) return;
    const std::filesystem::path real = std::filesystem::path(sysdir) / L"dxgi.dll";
    g_real = LoadLibraryW(real.c_str());
    if (!g_real) return;

    pCreateDXGIFactory = reinterpret_cast<PFN_CreateDXGIFactory>(GetProcAddress(g_real, "CreateDXGIFactory"));
    pCreateDXGIFactory1 = reinterpret_cast<PFN_CreateDXGIFactory1>(GetProcAddress(g_real, "CreateDXGIFactory1"));
    pCreateDXGIFactory2 = reinterpret_cast<PFN_CreateDXGIFactory2>(GetProcAddress(g_real, "CreateDXGIFactory2"));
    pDXGIDeclareAdapterRemovalSupport =
        reinterpret_cast<PFN_DXGIDeclareAdapterRemovalSupport>(GetProcAddress(g_real, "DXGIDeclareAdapterRemovalSupport"));
    pDXGIGetDebugInterface =
        reinterpret_cast<PFN_DXGIGetDebugInterface>(GetProcAddress(g_real, "DXGIGetDebugInterface"));
    pDXGIGetDebugInterface1 =
        reinterpret_cast<PFN_DXGIGetDebugInterface1>(GetProcAddress(g_real, "DXGIGetDebugInterface1"));
}

static DWORD WINAPI LoaderThread(LPVOID self_) {
    const auto self = static_cast<HMODULE>(self_);
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

    const std::wstring dir = GetSelfDir(self);
    const std::filesystem::path full = std::filesystem::path(dir) / L"hook.dll";
    LoadLibraryW(full.c_str());

    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        LoadRealDxgi();
        if (HANDLE h = CreateThread(nullptr, 0, LoaderThread, hModule, 0, nullptr)) {
            CloseHandle(h);
        }
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_real) FreeLibrary(g_real);
    }
    return TRUE;
}

extern "C" {

    __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory) {
        LoadRealDxgi();
        if (!pCreateDXGIFactory) return E_FAIL;
        return pCreateDXGIFactory(riid, ppFactory);
    }
    __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory) {
        LoadRealDxgi();
        if (!pCreateDXGIFactory1) return E_NOINTERFACE;
        return pCreateDXGIFactory1(riid, ppFactory);
    }
    __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void** ppFactory) {
        LoadRealDxgi();
        if (!pCreateDXGIFactory2) return E_NOINTERFACE;
        return pCreateDXGIFactory2(flags, riid, ppFactory);
    }
    __declspec(dllexport) HRESULT WINAPI DXGIDeclareAdapterRemovalSupport() {
        LoadRealDxgi();
        if (!pDXGIDeclareAdapterRemovalSupport) return E_NOTIMPL;
        return pDXGIDeclareAdapterRemovalSupport();
    }
    __declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface(REFIID riid, void** ppv) {
        LoadRealDxgi();
        if (!pDXGIGetDebugInterface) return E_NOINTERFACE;
        return pDXGIGetDebugInterface(riid, ppv);
    }
    __declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface1(UINT flags, REFIID riid, void** ppv) {
        LoadRealDxgi();
        if (!pDXGIGetDebugInterface1) return E_NOINTERFACE;
        return pDXGIGetDebugInterface1(flags, riid, ppv);
    }}