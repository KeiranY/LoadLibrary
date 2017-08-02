#include <Windows.h>

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

		MessageBox(nullptr, "Hello World!", "DllMain Injected", MB_OK);
		break;

	case DLL_PROCESS_DETACH:

		break;
	}


	return TRUE;
}
