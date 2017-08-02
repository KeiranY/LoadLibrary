#include <Windows.h>
#include <TlHelp32.h>
#include <chrono>
#include <thread>

#define PROCESS_OPEN_FLAGS PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_VM_OPERATION

DWORD ProcessID(const char* ProcessName)
{
	// First of all we create a snapshot handle specific for processes
	// (notice the usage of TH32CS_SNAPPROCESS) so we are able to call Process32First/Next
	// Remeber to close it when you don't use it anymore!
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
	// Check if the snapshot created is valid
	if (hSnapshot == INVALID_HANDLE_VALUE) return false;

	// Create the helper struct that will contain all the infos about the current process
	// while we loop through all the running processes
	PROCESSENTRY32 ProcEntry;
	// Remember to set the dwSize member of ProcEntry to sizeof(PROCESSENTRY32)
	ProcEntry.dwSize = sizeof(PROCESSENTRY32);

	DWORD returnID = 0;

	// Call Process32First
	if (Process32First(hSnapshot, &ProcEntry)) {
		do
		{
			// Notice that you have to enable Multi-Byte character set in order
			// to avoid converting everything.
			// strcmp is not the only way to compare 2 strings ofc, work with your imagination
			if (!strcmp(ProcEntry.szExeFile, ProcessName))
			{
				// If we are here it means that the process has been found
				// Store the process id into m_dwProcessId
				returnID = ProcEntry.th32ProcessID;
				// Return true meaning success
			}
		} while (Process32Next(hSnapshot, &ProcEntry));
	}

	CloseHandle(hSnapshot);
	return returnID;
}

void ErrorHandling(const char* FunctionName, const char* Message = "")
{
	if (strcmp(Message, "") == 0) {
		char err[256];
		DWORD code = GetLastError();
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, code,
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, nullptr);
		printf("%s: %s", FunctionName, err);
		std::this_thread::sleep_for(std::chrono::milliseconds(3000));
		exit(code);
	}
	else
	{
		printf("%s: %s", FunctionName, Message);
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

int main()
{
	//Get the ID of the csgo process
	//TODO: config to allow different processes
	DWORD processID = ProcessID("csgo.exe");
	//TODO: Actual error reporting
	if (!processID) ErrorHandling("ProcessID", "Process ID not found");
	//Get the full path of our .dll
	char dll[MAX_PATH];
	DWORD PathNameResult = GetFullPathName("inject.dll", MAX_PATH, dll, nullptr);
	if (!PathNameResult) ErrorHandling("GetFullPathName");
	if (PathNameResult > MAX_PATH) ErrorHandling("GetFullPathName", "Path Length too short");
	if (!FileExists(dll)) ErrorHandling("FileExists", "Dll to inject does not exist");

	//Get a handle to the process
	HANDLE Process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, processID);
	if (!Process) ErrorHandling("OpenProcess");

	// Allocate space in the process for our DLL 
	LPVOID Memory = LPVOID(VirtualAllocEx(Process, nullptr, MAX_PATH, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
	if (!Memory) ErrorHandling("VirtualAllocEx");
	// Write the string name of our DLL in the memory allocated 
	if (!WriteProcessMemory(Process, Memory, dll, MAX_PATH	, nullptr)) ErrorHandling("WriteProcessMemory");

	// Load our DLL
	FARPROC loadLibraryAddr = GetProcAddress(GetModuleHandle("kernel32.dl"), "LoadLibraryA");
	HANDLE hThread = CreateRemoteThread(Process, nullptr, NULL, LPTHREAD_START_ROUTINE(loadLibraryAddr), Memory, NULL, nullptr);
	if (!hThread) ErrorHandling("CreateRemoteThread");

	DWORD dwExit;
	WaitForSingleObject(hThread, INFINITE);
	GetExitCodeThread(hThread, &dwExit);

	//Let the program regain control of itself.
	CloseHandle(Process);

	//Free the allocated memory.
	VirtualFreeEx(Process, LPVOID(Memory), 0, MEM_RELEASE);

	return 0;
}