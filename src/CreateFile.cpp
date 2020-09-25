
#pragma comment(lib, "detours.lib")

#undef UNICODE
#include <windows.h>
#include <fstream>
#include <string>
#include <atlstr.h>
#include <direct.h>
#include <strsafe.h>
#include "./Detours/detours.h"
#include "d3d11/main.h"
#include "sp/environment.h"
#include "sp/main/preferences.h"

std::string GetExtensionA(LPCSTR Path);
std::wstring GetExtensionW(LPCWSTR Path);
BOOL RunCommandA(CAtlString cmdLine, DWORD& exitCode);
BOOL RunCommandW(CAtlStringW cmdLine, DWORD& exitCode);
HWND find_main_window();
HWND find_main_window(unsigned long ProcessID);
bool CmpStrA(std::string& str1, std::string& str2);
bool CmpStrW(std::wstring& str1, std::wstring& str2);
bool CmpCharA(char& c1, char& c2);
bool CmpCharW(WCHAR& c1, WCHAR& c2);
BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam);
BOOL is_main_window(HWND handle);

HANDLE(WINAPI* Real_CreateFileW) (
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile) = CreateFileW;

HANDLE(WINAPI* Real_CreateFileA) (
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile) = CreateFileA;

HANDLE WINAPI Routed_CreateFileW(
	LPCWSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{
	const int VideoCount = 6;
	const WCHAR* Videos[VideoCount] = { L"wmv", L"mpeg", L"mpg", L"mp4", L"mkv", L"avi" };

	auto Extension = GetExtensionW(lpFileName);

	for (int i = 0; i < VideoCount; i++) {
		auto FileExt = std::wstring(Videos[i]);
		if (CmpStrW(Extension, FileExt)) {
			CStringW Executable = CStringW(L"UnixShell.exe ");
			CStringW Arguments = CStringW(L"mpv --title=WINE --no-osc --ontop --fs --keep-open-pause=no --keep-open=no ");
			CStringW File = CStringW(lpFileName);
			auto Command = Executable + Arguments + File;
			HWND MainWindow = find_main_window();
			ShowWindow(MainWindow, SW_HIDE);
			DWORD ExitCode = NULL;
			RunCommandW(Command, ExitCode);
			ShowWindow(MainWindow, SW_SHOW);
			SetActiveWindow(MainWindow);
			break;
		}
	}

	return Real_CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}


HANDLE WINAPI Routed_CreateFileA(
	LPCSTR lpFileName,
	DWORD dwDesiredAccess,
	DWORD dwShareMode,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition,
	DWORD dwFlagsAndAttributes,
	HANDLE hTemplateFile)
{


	const int VideoCount = 6;
	const CHAR* Videos[VideoCount] = { "wmv", "mpeg", "mpg", "mp4", "mkv", "avi" };

	auto Extension = GetExtensionA(lpFileName);

	for (int i = 0; i < VideoCount; i++) {
		auto FileExt = std::string(Videos[i]);
		if (CmpStrA(Extension, FileExt)) {
			CString Executable = CString("UnixShell.exe ");
			CString Arguments = CString("mpv --title=UnixShell --no-osc --ontop --fs --keep-open-pause=no --keep-open=no ");
			CString File = CString(lpFileName);
			auto Command = Executable + Arguments + File;
			HWND MainWindow = find_main_window();
			ShowWindow(MainWindow, SW_HIDE);
			DWORD ExitCode = NULL;
			RunCommandA(Command, ExitCode);
			ShowWindow(MainWindow, SW_SHOW);
			SetActiveWindow(MainWindow);
			break;
		}
	}

	return Real_CreateFileA(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

std::string GetExtensionA(LPCSTR Path) {
	std::string Str = Path;
	return Str.substr(Str.find_last_of(".") + 1);
}


std::wstring GetExtensionW(LPCWSTR Path) {
	std::wstring Str = Path;
	return Str.substr(Str.find_last_of(L".") + 1);
}

BOOL RunCommandA(CAtlString cmdLine, DWORD& exitCode)
{
	PROCESS_INFORMATION processInformation = { 0 };
	STARTUPINFO startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	int nStrBuffer = cmdLine.GetLength() + 50;


	BOOL result = CreateProcessA(NULL, cmdLine.GetBuffer(nStrBuffer),
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &processInformation);
	cmdLine.ReleaseBuffer();


	if (!result)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();

		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		LocalFree(lpMsgBuf);
		return FALSE;
	}
	else
	{
		WaitForSingleObject(processInformation.hProcess, INFINITE);
		result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);

		if (!result)
		{
			return FALSE;
		}

		return TRUE;
	}
}

BOOL RunCommandW(CAtlStringW cmdLine, DWORD& exitCode)
{
	PROCESS_INFORMATION processInformation = { 0 };
	STARTUPINFOW startupInfo = { 0 };
	startupInfo.cb = sizeof(startupInfo);
	int nStrBuffer = cmdLine.GetLength() + 50;


	BOOL result = CreateProcessW(NULL, cmdLine.GetBuffer(nStrBuffer),
		NULL, NULL, FALSE,
		NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,
		NULL, NULL, &startupInfo, &processInformation);
	cmdLine.ReleaseBuffer();

	if (!result)
	{
		LPVOID lpMsgBuf;
		DWORD dw = GetLastError();
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);

		LocalFree(lpMsgBuf);

		return FALSE;
	}
	else
	{
		WaitForSingleObject(processInformation.hProcess, INFINITE);

		result = GetExitCodeProcess(processInformation.hProcess, &exitCode);

		CloseHandle(processInformation.hProcess);
		CloseHandle(processInformation.hThread);

		if (!result)
		{
			return FALSE;
		}

		return TRUE;
	}
}

struct handle_data {
	unsigned long process_id;
	HWND window_handle;
};

HWND find_main_window() {
	return find_main_window(GetCurrentProcessId());
}

HWND find_main_window(unsigned long process_id)
{
	handle_data data;
	data.process_id = process_id;
	data.window_handle = 0;
	EnumWindows(enum_windows_callback, (LPARAM)&data);
	return data.window_handle;
}

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
	handle_data& data = *(handle_data*)lParam;
	unsigned long process_id = 0;
	GetWindowThreadProcessId(handle, &process_id);
	if (data.process_id != process_id || !is_main_window(handle))
		return TRUE;
	data.window_handle = handle;
	return FALSE;
}

BOOL is_main_window(HWND handle)
{
	return GetWindow(handle, GW_OWNER) == (HWND)0 && IsWindowVisible(handle);
}

bool CmpCharA(char& c1, char& c2)
{
	if (c1 == c2)
		return true;
	else if (std::toupper(c1) == std::toupper(c2))
		return true;
	return false;
}
bool CmpCharW(WCHAR& c1, WCHAR& c2)
{
	if (c1 == c2)
		return true;
	else if (std::toupper(c1) == std::toupper(c2))
		return true;
	return false;
}

bool CmpStrA(std::string& str1, std::string& str2)
{
	return ((str1.size() == str2.size()) &&
		std::equal(str1.begin(), str1.end(), str2.begin(), &CmpCharA));
}

bool CmpStrW(std::wstring& str1, std::wstring& str2)
{
	return ((str1.size() == str2.size()) &&
		std::equal(str1.begin(), str1.end(), str2.begin(), &CmpCharW));
}

BOOL APIENTRY CreateFileMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	LONG Error;
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DetourRestoreAfterWith();
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(PVOID&)Real_CreateFileW, Routed_CreateFileW);
		DetourAttach(&(PVOID&)Real_CreateFileA, Routed_CreateFileA);
		Error = DetourTransactionCommit();

		if (Error == NO_ERROR)
			OutputDebugString(TEXT("Hooked Success"));
		else
			OutputDebugString(TEXT("Hook Error"));

		break;
	case DLL_PROCESS_DETACH:
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(PVOID&)Real_CreateFileW, Routed_CreateFileW);
		DetourDetach(&(PVOID&)Real_CreateFileA, Routed_CreateFileA);
		Error = DetourTransactionCommit();

		if (Error == NO_ERROR)
			OutputDebugString(TEXT("Un-Hooked Success"));
		else
			OutputDebugString(TEXT("Un-Hook Error"));
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	}
	return TRUE;
}

