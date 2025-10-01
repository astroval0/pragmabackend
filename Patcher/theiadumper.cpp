#include <windows.h>
#include <winnt.h>

extern "C" __declspec(dllexport)
BOOL __stdcall DecryptPage(void* pageAddr) {
	CONTEXT ctx{};
	ctx.ContextFlags = CONTEXT_FULL;
	ctx.Rip = reinterpret_cast<DWORD64>(pageAddr);
	ctx.Rsp = reinterpret_cast<DWORD64>(&ctx);
	ctx.Rbp = ctx.Rsp;
	
	DWORD64 imageBase = 0;
	DWORD64 establisher = 0;
	PVOID handler = nullptr;

	__try {
		auto fn = RtlLookupFunctionEntry(ctx.Rip, &imageBase, nullptr);

		RtlVirtualUnwind(UNW_FLAG_EHANDLER, imageBase, ctx.Rip, fn, &ctx, &handler, &establisher, nullptr);
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		// exception makes theia decrypt page
	}
	return TRUE;
}

extern "C" __declspec(dllexport)
BOOL __stdcall DecryptRange(void* startAddr, SIZE_T numPages) {
	BYTE* base = static_cast<BYTE*>(startAddr);
	for (SIZE_T i = 0; i < numPages; i++) {
		DecryptPage(base + i * 0x1000);
	}
	return TRUE;
}

extern "C" __declspec(dllexport)
BOOL __stdcall TouchPage(void* pageAddr) {
	__try {
		volatile auto b = *static_cast<BYTE*>(pageAddr);
		(void)b;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return FALSE;
	}
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hInst, DWORD reason, LPVOID) {
	if (reason == DLL_PROCESS_ATTACH) {
		DisableThreadLibraryCalls(hInst);
	}
	return TRUE;
}