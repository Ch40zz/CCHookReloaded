/* Custom injector to load the cheat DLL before the game runs its first instruction
   Supports simple injection via LdrLoadDll and advanced injection via manual-mapping.
*/

#include "framework.h"

#pragma comment(lib, "ntdll.lib")

std::filesystem::path GetExePath()
{
	wchar_t exePath[MAX_PATH + 1];
	GetModuleFileNameW(nullptr, exePath, std::size(exePath) - 1);
	return exePath;
}
void ShowError(const wchar_t *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	wchar_t buffer[1024];
	vswprintf_s(buffer, fmt, va);
	MessageBoxW(0, buffer, L"Error", MB_ICONERROR | MB_OK);

	va_end(va);
}
HANDLE GetParentProcess()
{
	PROCESS_BASIC_INFORMATION pbi;
	if (!NT_SUCCESS(NtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), nullptr)))
		return nullptr;

	const DWORD parentProcessId = (DWORD)pbi.Reserved3;
	return OpenProcess(PROCESS_CREATE_PROCESS, false, parentProcessId);
}
PPROC_THREAD_ATTRIBUTE_LIST CreatePpidSpoofedProcAttributeList(HANDLE *pSpoofedParentHandle)
{
	SIZE_T attributeListSize = 0;
	(void)InitializeProcThreadAttributeList(nullptr, 1, 0, &attributeListSize);

	PPROC_THREAD_ATTRIBUTE_LIST attributeList = (PPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(GetProcessHeap(), 0, attributeListSize);
	if (!InitializeProcThreadAttributeList(attributeList, 1, 0, &attributeListSize))
		return nullptr;

	if (!UpdateProcThreadAttribute(attributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, pSpoofedParentHandle, sizeof(HANDLE), nullptr, nullptr))
		return nullptr;

	return attributeList;
}
std::wstring Utf8To16(std::string_view utf8)
{
	std::wstring utf16(utf8.length(), L'\0');
	(void)MultiByteToWideChar(CP_OEMCP, 0, utf8.data(), static_cast<int>(utf8.length()), utf16.data(), static_cast<int>(utf16.length()));
	return utf16;
}
bool Is32BitProcess(HANDLE processHandle)
{
	typedef BOOL(WINAPI* tIsWow64Process)(HANDLE hProcess, PBOOL Wow64Process);
	tIsWow64Process pIsWow64Process = (tIsWow64Process)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");

	BOOL IsWow64;
	if (pIsWow64Process && pIsWow64Process(processHandle, &IsWow64) && !IsWow64)
		return false;

	return true;
}
HMODULE GetRemoteModuleHandle(HANDLE processHandle, std::wstring_view moduleName)
{
	HMODULE moduleHandle = nullptr;

	HANDLE snapHandle = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetProcessId(processHandle));
	if (snapHandle == INVALID_HANDLE_VALUE)
	{
		ShowError(L"CreateToolhelp32Snapshot failed (%lu)", GetLastError());
		return nullptr;
	}


	MODULEENTRY32W moduleEntry;
	moduleEntry.dwSize = sizeof(moduleEntry);

	if (!Module32FirstW(snapHandle, &moduleEntry))
	{
		ShowError(L"Module32First failed (%lu)", GetLastError());
		CloseHandle(snapHandle);
		return nullptr;
	}

	do
	{
		if (!_wcsnicmp(moduleEntry.szModule, moduleName.data(), moduleName.length()))
		{
			moduleHandle = (HMODULE)moduleEntry.modBaseAddr;
			break;
		}
	} while (Module32NextW(snapHandle, &moduleEntry));

	CloseHandle(snapHandle);

	return moduleHandle;
}

bool InjectDllLdrLoadDll(HANDLE processHandle, std::wstring_view dllPath)
{
	// Inject shellcode to call LdrLoadDll.
	// Kernel32 is not yet loaded at this point so we can't go the easy route with LoadLibraryA/W.

	if (!Is32BitProcess(processHandle))
	{
		ShowError(L"InjectDllLdrLoadDll: Target process is not 32-bit");
		return false;
	}

	uint8_t* mem = (uint8_t*)VirtualAllocEx(processHandle, nullptr, 0x2000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!mem)
	{
		ShowError(L"InjectDllLdrLoadDll: Failed to allocate memory (%lu)", GetLastError());
		return false;
	}

	uint8_t* unistring = mem;
	uint8_t* shellcode = mem + 0x1000;

	uint8_t unicodeData[0x1000] = {};
	UNICODE_STRING *unicodeString = (UNICODE_STRING*)&unicodeData;
	const uint16_t unicodeLen = (uint16_t)(dllPath.size() * sizeof(wchar_t));
	memcpy(unicodeString + 1, dllPath.data(), unicodeLen);
	unicodeString->Buffer = (wchar_t*)(unistring + sizeof(UNICODE_STRING));
	unicodeString->Length = unicodeString->MaximumLength = unicodeLen;

	if (!WriteProcessMemory(processHandle, unistring, unicodeData, sizeof(unicodeData), nullptr))
	{
		ShowError(L"InjectDllLdrLoadDll: Failed to write DLL path (%lu)", GetLastError());
		return false;
	}

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
	{
		ShowError(L"InjectDllLdrLoadDll: Failed to write shellcode (%lu)", GetLastError());
		return false;
	}
	
	HANDLE threadHandle = CreateRemoteThread(processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)shellcode, nullptr, 0, nullptr);
	if (!threadHandle)
	{
		ShowError(L"InjectDllLdrLoadDll: Failed to create remote thread (%lu)", GetLastError());
		return false;
	}

	(void)WaitForSingleObject(threadHandle, INFINITE);

	DWORD exitCode = ~0ul;
	if (!GetExitCodeThread(threadHandle, &exitCode) || !NT_SUCCESS(exitCode))
	{
		ShowError(L"InjectDllLdrLoadDll: LdrLoadDll was unsuccessful and returned status 0x%8X", exitCode);
		return false;
	}

	(void)CloseHandle(threadHandle);
	(void)VirtualFreeEx(processHandle, mem, 0, MEM_RELEASE);

	return true;
}
bool InjectDllManualMap(HANDLE processHandle, std::wstring_view dllPath)
{
	const auto Rpm = [processHandle](void* address, void* buffer, size_t size) -> bool {
		SIZE_T read;
		bool result = ReadProcessMemory(processHandle, address, buffer, size, &read);
		if (!result || read != size)
			ShowError(L"InjectDllManualMap: ReadProcessMemory failed (%lu)", GetLastError());

		return result;
	};
	const auto Wpm = [processHandle](void* address, void* buffer, size_t size) -> bool {
		SIZE_T read;
		bool result = WriteProcessMemory(processHandle, address, buffer, size, &read);
		if (!result || read != size)
			ShowError(L"InjectDllManualMap: WriteProcessMemory failed (%lu)", GetLastError());

		return result;
	};
	const auto Rva2Off = [](void* image, uint32_t rva) -> uint32_t {
		IMAGE_DOS_HEADER* idh = (IMAGE_DOS_HEADER*)image;
		IMAGE_NT_HEADERS* inh = (IMAGE_NT_HEADERS*)((uint8_t*)image + idh->e_lfanew);
		IMAGE_SECTION_HEADER* SectionHeader = IMAGE_FIRST_SECTION(inh);

		for (size_t i = 0; i < inh->FileHeader.NumberOfSections; i++)
		{
			if (rva >= SectionHeader[i].VirtualAddress &&
				rva < SectionHeader[i].VirtualAddress + SectionHeader[i].Misc.VirtualSize)
				return rva - SectionHeader[i].VirtualAddress + SectionHeader[i].PointerToRawData;
		}

		return 0;
	};
	const auto Off2Rva = [](void* image, uint32_t off) -> uint32_t {
		IMAGE_DOS_HEADER* idh = (IMAGE_DOS_HEADER*)image;
		IMAGE_NT_HEADERS* inh = (IMAGE_NT_HEADERS*)((uint8_t*)image + idh->e_lfanew);
		IMAGE_SECTION_HEADER* SectionHeader = IMAGE_FIRST_SECTION(inh);

		for (size_t i = 0; i < inh->FileHeader.NumberOfSections; i++)
		{
			if (off >= SectionHeader[i].PointerToRawData &&
				off < SectionHeader[i].PointerToRawData + SectionHeader[i].SizeOfRawData)
				return off - SectionHeader[i].PointerToRawData + SectionHeader[i].VirtualAddress;
		}

		return 0;
	};

	if (!Is32BitProcess(processHandle))
	{
		ShowError(L"InjectDllManualMap: Target process is not 32-bit");
		return false;
	}

	// Read file from disk and get the PE Headers

	const HANDLE fileHandle = CreateFileW(std::wstring(dllPath).data(), 
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);

	if (fileHandle == INVALID_HANDLE_VALUE)
	{
		ShowError(L"InjectDllManualMap: CreateFileW failed to open the image (%lu)", GetLastError());
		return false;
	}

	DWORD fileSize = GetFileSize(fileHandle, nullptr);

	uint8_t* pImage = (uint8_t*)VirtualAlloc(nullptr, fileSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (!pImage)
	{
		ShowError(L"InjectDllManualMap: VirtualAlloc failed to allocate image (%lu)", GetLastError());
		return false;
	}

	if (!ReadFile(fileHandle, pImage, fileSize, &fileSize, nullptr))
	{
		ShowError(L"InjectDllManualMap: ReadFile failed to read the image (%lu)", GetLastError());
		return false;
	}

	auto* idh = (IMAGE_DOS_HEADER*)pImage;
	auto* inh = (IMAGE_NT_HEADERS*)(pImage + idh->e_lfanew);
	auto* ish = IMAGE_FIRST_SECTION(inh);

	// Allocate the remote image memory if not already done
	void *pMemory = VirtualAllocEx(processHandle, nullptr, inh->OptionalHeader.SizeOfImage, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
	if (!pMemory)
	{
		ShowError(L"InjectDllManualMap: VirtualAllocEx failed to allocate the image (%lu)", GetLastError());
		return false;
	}

	// Copy all sections to the remote process, do not copy PE Headers
	for (size_t i = 0; i < inh->FileHeader.NumberOfSections; i++)
	{
		if (!Wpm((uint8_t*)pMemory + ish[i].VirtualAddress, pImage + ish[i].PointerToRawData, ish[i].SizeOfRawData))
			return false;
	}

	// Handle relocations
	{
		IMAGE_DATA_DIRECTORY* relocDir = &inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
		if (inh->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_BASERELOC && relocDir->VirtualAddress)
		{
			auto* relocData = (IMAGE_BASE_RELOCATION*)(pImage + Rva2Off(pImage, relocDir->VirtualAddress));
			const int64_t relocDelta = int64_t((uintptr_t(pMemory) - inh->OptionalHeader.ImageBase));

			while (relocData->SizeOfBlock >= sizeof(IMAGE_BASE_RELOCATION))
			{
				const uint32_t recordCount = (relocData->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(uint16_t);
				const uint16_t* relocItems = (uint16_t*)(relocData + 1);

				for (size_t i = 0; i < recordCount; i++)
				{
					const uint16_t relocType = relocItems[i] >> 12;
					const uint16_t relocOffset = relocItems[i] & 0xFFF;
					const uint32_t rva = relocData->VirtualAddress + relocOffset;

					switch (relocType)
					{
					case IMAGE_REL_BASED_ABSOLUTE:
						// No fixup required
						break;
					case IMAGE_REL_BASED_HIGHLOW:
					{
						uint32_t NewValue = uint32_t(*(uint32_t*)(pImage + Rva2Off(pImage, rva)) + relocDelta);

						if (!Wpm((uint8_t*)pMemory + rva, &NewValue, sizeof(NewValue)))
							return false;

						break;
					}
					case IMAGE_REL_BASED_DIR64:
					{
						uint64_t NewValue = *(uint64_t*)(pImage + Rva2Off(pImage, rva)) + relocDelta;

						if (!Wpm((uint8_t*)pMemory + rva, &NewValue, sizeof(NewValue)))
							return false;

						break;
					}
					default:
						ShowError(L"InjectDllManualMap: Invalid relocation type (%u) encountered", relocType);
					}
				}

				relocData = (IMAGE_BASE_RELOCATION*)((uint8_t*)relocData + relocData->SizeOfBlock);
			}
		}
	}

	// Resolve IAT
	{
		IMAGE_DATA_DIRECTORY* importDir = &inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
		if (inh->OptionalHeader.NumberOfRvaAndSizes >= IMAGE_DIRECTORY_ENTRY_IMPORT && importDir->VirtualAddress)
		{
			auto* importDesc = (IMAGE_IMPORT_DESCRIPTOR*)(pImage + Rva2Off(pImage, importDir->VirtualAddress));

			for (; importDesc->Characteristics; importDesc++)
			{
				const char* moduleName = (char*)pImage + Rva2Off(pImage, importDesc->Name);

				const HMODULE localModHandle = LoadLibraryExA(moduleName, nullptr, DONT_RESOLVE_DLL_REFERENCES);
				if (!localModHandle)
				{
					ShowError(L"InjectDllManualMap: Could not find local import '%S'", moduleName);
					return false;
				}

				const auto moduleNameUtf16 = Utf8To16(moduleName);

				if (!InjectDllLdrLoadDll(processHandle, moduleNameUtf16))
				{
					ShowError(L"InjectDllManualMap: Failed to inject missing import '%S'", moduleName);
					return false;
				}

				const HMODULE remoteModHandle = GetRemoteModuleHandle(processHandle, moduleNameUtf16);
				if (!remoteModHandle)
				{
					ShowError(L"InjectDllManualMap: Failed to inject missing import '%S'", moduleName);
					return false;
				}

				const auto* origfirstThunk = (IMAGE_THUNK_DATA*)(pImage + Rva2Off(pImage, importDesc->OriginalFirstThunk));
				const auto* firstThunk = (IMAGE_THUNK_DATA*)(pImage + Rva2Off(pImage, importDesc->FirstThunk));

				while (origfirstThunk->u1.AddressOfData)
				{
					const char* importName = nullptr;
					if (origfirstThunk->u1.Ordinal & IMAGE_ORDINAL_FLAG)
					{
						importName = (char*)(origfirstThunk->u1.Ordinal & 0xFFFF);
					}
					else
					{
						IMAGE_IMPORT_BY_NAME* nameImport = (IMAGE_IMPORT_BY_NAME*)(pImage + Rva2Off(pImage, uint32_t(origfirstThunk->u1.AddressOfData)));
						importName = nameImport->Name;
					}

					uintptr_t localFunction = (uintptr_t)GetProcAddress(localModHandle, importName);
					if (!localFunction)
					{
						ShowError(L"InjectDllManualMap: Failed to locally resolve API for module '%S'", moduleName);
						return false;
					}

					uintptr_t resolvedFunction = localFunction - uintptr_t(localModHandle) + uintptr_t(remoteModHandle);

					// firstThunk uses pImage which is not fully mapped to memory, thus all offsets are file offsets and not virtual
					uint32_t remoteFunctionOff = uint32_t((uint8_t*)(&firstThunk->u1.Function) - pImage);
					void* remoteFunction = Off2Rva(pImage, remoteFunctionOff) + (uint8_t*)pMemory;

					// firstThunk->u1.Function = GetProcAddress(importName);
					if (!Wpm(remoteFunction, &resolvedFunction, sizeof(resolvedFunction)))
						return false;

					origfirstThunk++;
					firstThunk++;
				}

				(void)FreeLibrary(localModHandle);
			}
		}
	}

	// Overwrite start of PE Header  with Shellcode to call DllMain with correct params

	uint8_t dllMainShellcode[] = {
		0x68, 0, 0, 0, 0,
		0x68, 0, 0, 0, 0,
		0x68, 0, 0, 0, 0,
		0xB8, 0, 0, 0, 0,
		0xFF, 0xD0,
		0xC3
	};

	*(uint32_t*)(dllMainShellcode + 0x01) = 0;
	*(uint32_t*)(dllMainShellcode + 0x06) = 0x01;
	*(uint32_t*)(dllMainShellcode + 0x0B) = (uintptr_t)pMemory;
	*(uint32_t*)(dllMainShellcode + 0x10) = (uintptr_t)pMemory + inh->OptionalHeader.AddressOfEntryPoint;

	if (!Wpm(pMemory, dllMainShellcode, sizeof(dllMainShellcode)))
		return false;

	const HANDLE remoteThread = CreateRemoteThread(processHandle, nullptr, 0, (LPTHREAD_START_ROUTINE)pMemory, nullptr, 0, nullptr);
	if (!remoteThread)
	{
		ShowError(L"InjectDllLdrLoadDll: Failed to create remote thread!");
		return false;
	}

	(void)WaitForSingleObject(remoteThread, INFINITE);

	DWORD exitCode = ~0ul;
	if (!GetExitCodeThread(remoteThread, &exitCode) || exitCode != TRUE)
	{
		ShowError(L"InjectDllManualMap: DllMain was unsuccessful and returned status 0x%8X", exitCode);
		return false;
	}

	memset(dllMainShellcode, 0, sizeof(dllMainShellcode));
	if (!Wpm(pMemory, dllMainShellcode, sizeof(dllMainShellcode)))
		return false;

	(void)CloseHandle(remoteThread);
	(void)CloseHandle(fileHandle);
	(void)VirtualFree(pImage, 0, MEM_RELEASE);

	return true;
}

int APIENTRY wWinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPWSTR /*lpCmdLine*/, int /*nCmdShow*/)
{
	/* Basic flow of the injector:
	   1. Create process in suspended mode
	   2. Inject DLL with the selected method
	   3. Resume process
	*/

	wchar_t etExePath[MAX_PATH + 1] = { L"et.exe" };
	int injectionMethod = 0;

	const std::filesystem::path cfgPath = GetExePath().replace_extension(L".ini");
	if (std::filesystem::exists(cfgPath))
	{
		GetPrivateProfileStringW(L"CCInject", L"EtExePath", etExePath, etExePath, std::size(etExePath) - 1, cfgPath.c_str());
		injectionMethod = GetPrivateProfileIntW(L"CCInject", L"InjectionMethod", injectionMethod, cfgPath.c_str());
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

		const int userChoice = MessageBoxW(0, L"Do you want to use the (probably safer) manual-mapping injection technique?\n"
											  L"If encountering any issues delete the .ini file and press no.", L"Use manual-mapping?", MB_YESNO | MB_ICONQUESTION);

		injectionMethod = (userChoice == IDYES) ? 1 : 0;

		WritePrivateProfileStringW(L"CCInject", L"EtExePath", etExePath, cfgPath.c_str());
		WritePrivateProfileStringW(L"CCInject", L"InjectionMethod", std::to_wstring(injectionMethod).c_str(), cfgPath.c_str());
	}

	
	PROCESS_INFORMATION pi = {};
	STARTUPINFOEXW si = { sizeof(si) };
	HANDLE parentProcessHandle = GetParentProcess();
	si.lpAttributeList = CreatePpidSpoofedProcAttributeList(&parentProcessHandle);

	std::filesystem::path etExePathFs = { etExePath };
	if (!CreateProcessW(nullptr, etExePath, nullptr, nullptr, false, CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT, nullptr, etExePathFs.parent_path().c_str(), &si.StartupInfo, &pi))
	{
		ShowError(L"Failed to create process (%lu)", GetLastError());
		return 1;
	}

	const std::wstring dllPath = GetExePath().replace_extension(L".dll").wstring();
	const bool success = injectionMethod == 0
					? InjectDllLdrLoadDll(pi.hProcess, dllPath)
					: InjectDllManualMap(pi.hProcess, dllPath);
	if (!success)
	{
		TerminateProcess(pi.hProcess, 1);
		return 1;
	}

	if (ResumeThread(pi.hThread) == ~0ul)
	{
		ShowError(L"Failed to resume the process (%lu)", GetLastError());
		TerminateProcess(pi.hProcess, 1);
		return 1;
	}

	DeleteProcThreadAttributeList(si.lpAttributeList);
	(void)CloseHandle(parentProcessHandle);
	(void)CloseHandle(pi.hThread);
	(void)CloseHandle(pi.hProcess);

	return 0;
}
