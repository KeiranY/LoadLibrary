#include <Windows.h>
#include <Psapi.h>
#include <stdarg.h>
#include <thread>

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
