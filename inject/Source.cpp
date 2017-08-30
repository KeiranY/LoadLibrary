#include <Windows.h>
#include <Psapi.h>
#include <stdarg.h>
#include <chrono>
#include <thread>
#include <string>


BOOLEAN WINAPI DllMain(IN HINSTANCE, IN DWORD, IN LPVOID);

typedef void(__cdecl* MsgFn)(const char* msg);

MODULEINFO GetModuleInfo(char *szModule)
{
	MODULEINFO modinfo = { nullptr };
	HMODULE hModule = GetModuleHandle(szModule);
	if (hModule == nullptr)
		return modinfo;
	GetModuleInformation(GetCurrentProcess(), hModule, &modinfo, sizeof(MODULEINFO));
	return modinfo;
}



BOOLEAN WINAPI DllMain(IN HINSTANCE hDllHandle,
	IN DWORD     nReason,
	IN LPVOID    Reserved)
{

	//  Perform global initialization.
	switch (nReason)
	{
	case DLL_PROCESS_ATTACH:
		//  For optimization.
		DisableThreadLibraryCalls(hDllHandle);

		MessageBox(nullptr, "Success", "Dll Injected", MB_OK | MB_ICONINFORMATION);
		break;

	case DLL_PROCESS_DETACH:

		break;
	}

	return TRUE;
}
