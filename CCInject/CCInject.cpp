/* Simple LdrLoadDll injector to inject our cheat.
Should be undetected for all the custom Anti-Cheats and Punkbuster,
but if you want to be more secure you probably want to switch out the
injection logic with something more complex such as manualmapping. */

#include "framework.h"

bool InjectDll(HANDLE processHandle, std::wstring_view dllPath)
{
	// Inject shellcode to call LdrLoadDll.
	// Kernel32 is not yet loaded at this point so we can't go the easy route with LoadLibraryA/W.

	uint8_t* mem = (uint8_t*)VirtualAllocEx(processHandle, nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!mem)
		return false;

	uint8_t* unistring = mem;
	uint8_t* shellcode = mem + 0x1000;

	uint8_t unicodeData[0x1000] = {};
	UNICODE_STRING *unicodeString = (UNICODE_STRING*)&unicodeData;
	const uint16_t unicodeLen = (uint16_t)(dllPath.size() * sizeof(wchar_t));
	memcpy(unicodeString + 1, dllPath.data(), unicodeLen);
	unicodeString->Buffer = (wchar_t*)(unistring + sizeof(UNICODE_STRING));
	unicodeString->Length = unicodeString->MaximumLength = unicodeLen;

	if (!WriteProcessMemory(processHandle, unistring, unicodeData, sizeof(unicodeData), nullptr))
		return false;

	uint8_t shellcode_data[] = {
		0x68, 0, 0, 0, 0,	/* push <arg4 @ $+1> */
		0x68, 0, 0, 0, 0,	/* push <arg3 @ $+6> */
		0x68, 0, 0, 0, 0,	/* push <arg2 @ $+11> */
		0x68, 0, 0, 0, 0,	/* push <arg1 @ $+16> */
		0xB8, 0, 0, 0, 0,	/* mov eax, <calltarget @ $+21> */
		0xFF, 0xD0,			/* call eax */
		0xC3,				/* ret */
	};

	/* 1: PathToFile */ *(uintptr_t*)(shellcode_data + 16)		= 0;
	/* 2: Flags */ *(uintptr_t*)(shellcode_data + 11)			= 0;
	/* 3: ModuleFileName */ *(uint8_t**)(shellcode_data + 6)	= unistring;
	/* 4: DllHandle */ *(uint8_t**)(shellcode_data + 1)			= shellcode;
	/* LdrLoadDll() */ *(void**)(shellcode_data + 21)			= GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "LdrLoadDll");

	if (!WriteProcessMemory(processHandle, shellcode, shellcode_data, sizeof(shellcode_data), nullptr))
		return false;
	
	HANDLE threadHandle = CreateRemoteThread(processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)shellcode, nullptr, 0, nullptr);
	if (!threadHandle)
		return false;

	(void)WaitForSingleObject(threadHandle, INFINITE);

	DWORD exitCode = ~0ul;
	if (!GetExitCodeThread(threadHandle, &exitCode) || !NT_SUCCESS(exitCode))
	{
		wchar_t errormsg[128];
		wsprintf(errormsg, L"LdrLoadDll was unsuccessful and returned status 0x%8X", exitCode);
		MessageBoxW(0, errormsg, L"Error", MB_ICONERROR | MB_OK);

		return false;
	}

	(void)CloseHandle(threadHandle);
	(void)VirtualFreeEx(processHandle, mem, 0, MEM_RELEASE);

	return true;
}

int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	/* Basic flow of the injector:
	   1. Create process in suspended mode
	   2. Inject DLL using LdrLoadDll
	   3. Resume process
	*/

	wchar_t  startupPathBuff[MAX_PATH + 1];
	GetModuleFileNameW(nullptr, startupPathBuff, std::size(startupPathBuff) - 1);
	std::filesystem::path startupPath = { startupPathBuff };

	wchar_t etExePath[MAX_PATH + 1] = { L"et.exe" };
	std::filesystem::path cfgPath = startupPath.replace_extension(L".cfg");
	if (std::filesystem::exists(cfgPath))
	{
		std::wifstream cfgFile(cfgPath.c_str(), std::ifstream::in);
		cfgFile.getline(etExePath, std::size(etExePath), L'\0');
	}
	else
	{
		OPENFILENAME ofn = {};
		ofn.lStructSize = sizeof(ofn);
		ofn.lpstrFile = etExePath;
		ofn.nMaxFile = std::size(etExePath);
		ofn.lpstrFilter = L"All Files (*.*)\0*.*\0Executable Files (*.exe)\0*.exe\0";
		ofn.nFilterIndex = 2;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOTESTFILECREATE | OFN_READONLY | OFN_ENABLESIZING;
		if (!GetOpenFileNameW(&ofn))
			return 1;

		std::wofstream cfgFile(cfgPath.c_str(), std::ifstream::out);
		cfgFile.write(etExePath, wcslen(etExePath));
	}

	
	PROCESS_INFORMATION pi = {};
	STARTUPINFOW si = { sizeof(STARTUPINFOW) };
	std::filesystem::path exePath = { etExePath };
	if (!CreateProcessW(nullptr, etExePath, nullptr, nullptr, false, CREATE_SUSPENDED, nullptr, exePath.parent_path().c_str(), &si, &pi))
	{
		MessageBoxW(0, L"Failed to create process!", L"Error", MB_ICONERROR | MB_OK);
		return 1;
	}

	std::wstring dllPath = startupPath.replace_extension(L".dll").wstring();
	if (!InjectDll(pi.hProcess, dllPath))
	{
		TerminateProcess(pi.hProcess, 1);
		return 1;
	}

	typedef NTSTATUS (WINAPI *tNtResumeProcess)(HANDLE handle);
	tNtResumeProcess NtResumeProcess = (tNtResumeProcess)GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtResumeProcess");
	if (!NT_SUCCESS(NtResumeProcess(pi.hProcess)))
	{
		TerminateProcess(pi.hProcess, 1);
		return 1;
	}

	return 0;
}
