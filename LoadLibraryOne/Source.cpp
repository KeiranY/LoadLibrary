#include <windows.h>
#include <TlHelp32.h>
#include <chrono>
#include <thread>
#include <libloaderapi.h>
#include <ostream>

#define DEFAULT_CONFIG "[Options]\r\nprocess=\"calc.exe\"\r\ndll=\"inject.dll\""

DWORD ProcessID(const char* ProcessName)
{
	//Create a snapshot of all running processes
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	if (hSnapshot == INVALID_HANDLE_VALUE) return false;

	//Used to store the process info in the loop
	PROCESSENTRY32 ProcEntry;
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);

	//Get the first process
	if (Process32First(hSnapshot, &ProcEntry)) {
		do
		{
			//If the found process name is equal to the on we're searching for
			if (!strcmp(ProcEntry.szExeFile, ProcessName))
			{
				CloseHandle(hSnapshot);
				//Return the processID of the found process
				//TODO: return a list of found processes instead
				return ProcEntry.th32ProcessID;
			}
		} while (Process32Next(hSnapshot, &ProcEntry)); //Get the next process
	}

	CloseHandle(hSnapshot);
	//Since a process hasn't been found, return 0
	return 0;
}

void ErrorHandling(const char* FunctionName, const char* Message = "")
{
	if (strcmp(Message, "") == 0) {
		char err[256];
		DWORD code = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, nullptr);
		char out[256];
		sprintf(out, "%s: %s", FunctionName, err);
		MessageBox(nullptr, out, "LoadLibraryOne", MB_OK | MB_ICONSTOP);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		exit(code);
	}
	else
	{
		char out[256];
		sprintf(out, "%s: %s", FunctionName, Message);
		MessageBox(nullptr, out, "LoadLibraryOne", MB_OK | MB_ICONSTOP);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		exit(-1);
	}

}

BOOL FileExists(LPCTSTR szPath)
{
	DWORD dwAttrib = GetFileAttributes(szPath);

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

HMODULE GrabModule(DWORD processID, const char* strModuleName)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, processID);
	// Check if the snapshot created is valid
	if (hSnapshot == INVALID_HANDLE_VALUE) return nullptr;

	MODULEENTRY32 ModEntry;
	// Call Module32First
	if (Module32First(hSnapshot, &ModEntry))
	{
		do
		{
			// Notice that you have to enable Multi-Byte character set in order
			// to avoid converting everything.
			// strcmp is not the only way to compare 2 strings ofc, work with your imagination
			if (!strcmp(ModEntry.szModule, strModuleName))
			{
				// If we are here it means that the module has been found, we can add the module to the vector
				// But first of all we have to close the snapshot handle!
				CloseHandle(hSnapshot);
				// Add ModEntry to the m_Modules vector
				return HMODULE(ModEntry.modBaseAddr);
			}
		} while (Module32Next(hSnapshot, &ModEntry));
	}
	// If we are here it means that the module has not been found or that there are no modules to scan for anymore.
	// We can close the snapshot handle and return false.
	CloseHandle(hSnapshot);
	return nullptr;
}

int WinMain(HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPTSTR    lpCmdLine,
	int       cmdShow)
{
	char ini[256];
	GetCurrentDirectory(MAX_PATH, ini);
	strcat(ini, "\\config.ini");
	DWORD attributes = GetFileAttributes(ini);
	if((attributes == INVALID_FILE_ATTRIBUTES ||
		(attributes & FILE_ATTRIBUTE_DIRECTORY)))
	{
		HANDLE hFile = CreateFile(ini, GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
		if(!hFile || hFile == INVALID_HANDLE_VALUE)
			ErrorHandling("Ini file", "Config file not found. An attempt to create a config file was also unsuccessful");
		WriteFile(hFile, DEFAULT_CONFIG, sizeof(DEFAULT_CONFIG), nullptr, nullptr);
		ErrorHandling("Ini file", "Config file not found. A default config file has been created in the current directory.");
	}

	char process[255];
	char file[255];
	GetPrivateProfileString("Options", "process", nullptr, process, 255, ini);
	GetPrivateProfileString("Options", "dll", nullptr, file, 255, ini);
	if (process == "") ErrorHandling("Config", "Process name in config file is blank");
	if (file == "") ErrorHandling("Config", "DLL name in config file is blank");


	//Get the ID of the csgo process
	DWORD processID = ProcessID(process);
	if (!processID) ErrorHandling("ProcessID", "Process ID not found");
	//Get the full path of our .dll
	char dll[MAX_PATH];
	DWORD PathNameResult = GetFullPathName(file, MAX_PATH, dll, nullptr);
	if (!PathNameResult) ErrorHandling("GetFullPathName");
	if (PathNameResult > MAX_PATH) ErrorHandling("GetFullPathName", "Path Length too short");
	if (!FileExists(dll)) ErrorHandling("FileExists", "Dll to inject does not exist");
	if (GrabModule(processID, file)) ErrorHandling("GrabModule", "Dll already injected");

	//Get a handle to the process
	HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	if (!Process) ErrorHandling("OpenProcess");

	// Allocate space in the process for our DLL 
	LPVOID Memory = LPVOID(VirtualAllocEx(Process, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (!Memory) ErrorHandling("VirtualAllocEx");
	// Write the string name of our DLL in the memory allocated 
	if (!WriteProcessMemory(Process, Memory, dll, MAX_PATH	, nullptr)) ErrorHandling("WriteProcessMemory");

	// Load our DLL
	HANDLE hThread = CreateRemoteThread(Process, nullptr, NULL, LPTHREAD_START_ROUTINE(LoadLibraryA), Memory, NULL, nullptr);
	if (!hThread) ErrorHandling("CreateRemoteThread");

	//Let the program regain control of itself.
	CloseHandle(Process);

	//Free the allocated memory.
	VirtualFreeEx(Process, LPVOID(Memory), 0, MEM_RELEASE);

	MessageBox(nullptr, "File successfully injected", "LoadLibraryOne", MB_OK | MB_ICONINFORMATION);

	return 0;
}