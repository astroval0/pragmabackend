#include <windows.h>

HMODULE realDxgi = nullptr;
HMODULE theiadumper = nullptr;
HMODULE hook = nullptr;


// Helper macros to stringify and widen
#define WIDEN2(x) L##x
#define WIDEN(x) WIDEN2(x)

#define THEIADUMPER_DLL_PATH_LSTR WIDEN(THEIADUMPER_DLL_PATH)
#define HOOK_DLL_PATH_LSTR WIDEN(HOOK_DLL_PATH)

FARPROC LoadRealFunction(const char* name) {
	if (!realDxgi) {
		realDxgi = LoadLibraryW(L"C:\\Windows\\System32\\dxgi.dll");
	}
	if (!theiadumper) {
		theiadumper = LoadLibraryW(THEIADUMPER_DLL_PATH_LSTR);
	}
	if (!hook) {
		hook = LoadLibraryW(HOOK_DLL_PATH_LSTR);
	}
	return GetProcAddress(realDxgi, name);
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory(REFIID riid, void** ppFactory) {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(LoadRealFunction("CreateDXGIFactory"));
	return fn ? fn(riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory1(REFIID riid, void** ppFactory) {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(LoadRealFunction("CreateDXGIFactory1"));
	return fn ? fn(riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI CreateDXGIFactory2(UINT flags, REFIID riid, void** ppFactory) {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)(UINT, REFIID, void**)>(LoadRealFunction("CreateDXGIFactory2"));
	return fn ? fn(flags, riid, ppFactory) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIDeclareAdapterRemovalSupport() {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)()>(LoadRealFunction("DXGIDeclareAdapterRemovalSupport"));
	return fn ? fn() : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIDisableVBlankVirtualization() {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)()>(LoadRealFunction("DXGIDisableVBlankVirtualization"));
	return fn ? fn() : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface(REFIID riid, void** ppDebug) {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)(REFIID, void**)>(LoadRealFunction("DXGIGetDebugInterface"));
	return fn ? fn(riid, ppDebug) : E_FAIL;
}

extern "C" __declspec(dllexport) HRESULT WINAPI DXGIGetDebugInterface1(UINT flags, REFIID riid, void** ppDebug) {
	auto fn = reinterpret_cast<HRESULT(WINAPI*)(UINT, REFIID, void**)>(LoadRealFunction("DXGIGetDebugInterface1"));
	return fn ? fn(flags, riid, ppDebug) : E_FAIL;
}