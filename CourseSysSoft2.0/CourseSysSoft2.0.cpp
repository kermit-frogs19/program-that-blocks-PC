#include "framework.h"
#include "CourseSysSoft2.0.h"
#include <windows.h>
#include <winuser.h>
#include <iostream>
#include <tlhelp32.h>
#include <wchar.h>

using namespace std;

#define MAX_LOADSTRING 100

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam);
HKEY hKey;
LONG result;
HANDLE hSnapshot, hProcess;


bool TerminateExplorer() // function of terminating explorer.exe process to stop certain system key shortcuts
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0); // creating the snapshot of the process 
	if (hSnapshot == INVALID_HANDLE_VALUE) // to find explorer.exe later in the code
	{
		std::cerr << "Failed to create snapshot of processes." << std::endl; //outputing error messages
		return false;
	}

	PROCESSENTRY32 pe32;
	pe32.dwSize = sizeof(PROCESSENTRY32);

	if (!Process32First(hSnapshot, &pe32)) 
	{
		std::cerr << "Failed to get first process." << std::endl;
		CloseHandle(hSnapshot);
		return false;
	}

	bool explorerTerminated = false;

	do 
	{
		if (wcscmp(pe32.szExeFile, L"explorer.exe") == 0) 
		{
			HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
			if (hProcess == NULL) 
			{
				std::cerr << "Failed to open explorer.exe." << std::endl;
				CloseHandle(hSnapshot);
				return false;
			}

			if (!TerminateProcess(hProcess, 0)) 
			{
				std::cerr << "Failed to terminate explorer.exe." << std::endl;
				CloseHandle(hProcess);
			}
			else 
			{
				std::cout << "Explorer.exe terminated successfully." << std::endl;
				explorerTerminated = true;
			}

			CloseHandle(hProcess);
			break;
		}
	} while (Process32Next(hSnapshot, &pe32));

	CloseHandle(hSnapshot);
	return explorerTerminated;
}


bool StartExplorerMinimized()  // the function of starting minimised explorer process
{
	STARTUPINFO si = { sizeof(si) };
	PROCESS_INFORMATION pi;

	// Set the window to be minimized at startup
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOWMINIMIZED;

	// Path to Windows Explorer
	WCHAR explorerPath[] = L"explorer.exe";

	// Start the process
	if (!CreateProcess(NULL, explorerPath, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) 
	{
		std::cerr << "Failed to start explorer.exe minimized." << std::endl;
		return false;
	}

	// Close process and thread handles
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return true;
}


int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int response = MessageBox( // creating a message-window to informate the user
		NULL, 
		L"If you press OK the operating of your computer will be completely blocked.", 
		L"So you know :/", 
		MB_OKCANCEL | 
		MB_ICONINFORMATION);
	if (response == IDCANCEL)
	{
		ExitProcess(0);
	}
	MSG msg;
	BOOL bRet;
	HWND hwndMain;
	WNDCLASSEX wcx;

	(void)hPrevInstance;
	(void)lpCmdLine;
	(void)nCmdShow;

	wcx.cbSize = sizeof(wcx); // "styling" the window
	wcx.style = CS_HREDRAW | CS_VREDRAW;
	wcx.lpfnWndProc = WindowProc;
	wcx.cbClsExtra = 0;
	wcx.cbWndExtra = 0;
	wcx.hInstance = hInstance;
	wcx.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wcx.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcx.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	wcx.lpszMenuName = L"MainMenu";
	wcx.lpszClassName = L"MainWndClass";
	wcx.hIconSm = (HICON)LoadImage(
		hInstance,
		MAKEINTRESOURCE(5),
		IMAGE_ICON,
		GetSystemMetrics(SM_CXSMICON),
		GetSystemMetrics(SM_CYSMICON),
		LR_DEFAULTCOLOR
	);

	if (!RegisterClassEx(&wcx))
	{
		return FALSE;
	}

	hwndMain = CreateWindow( // creating the main window of the program
		L"MainWndClass", 
		L"No Windows Button",
		WS_ICONIC | WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		400,
		80,
		(HWND)NULL,
		(HMENU)NULL,
		hInstance,
		(LPVOID)NULL
	);

	if (!hwndMain)
	{
		return FALSE;
	}

	
	if (TerminateExplorer()) // terminating the process of explorer.exe to block certaing key shortcuts
	{
		cout << "Explorer closed" << endl;
	}
	else
	{
		cout << "Failed to close explorer" << endl;

	}
	AnimateWindow(hwndMain, 1000, AW_HIDE | AW_BLEND); // "backgrounding" the window
	UpdateWindow(hwndMain);
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0)
	{
		if (bRet == -1)
		{
			// handle the error and possibly exit
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	

	return msg.wParam;
}

static HHOOK hook_keys; // creating hooks to "hook" the key and mouse inputs
static HHOOK new_hook_keys;
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
	case WM_CREATE: // handling the creation of the window
		//installing the low level hooks
		hook_keys = SetWindowsHookEx(WH_KEYBOARD_LL, LLKeyboardProc, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		new_hook_keys = SetWindowsHookEx(WH_MOUSE_LL, LLMouseProc, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		return 0;

	case WM_DESTROY: // handling the destroying of the window (the only way to stop the program)
		UnhookWindowsHookEx(hook_keys); //unhooking the devices
		UnhookWindowsHookEx(new_hook_keys);
		if (StartExplorerMinimized()) // restarting the process of explorer.exe
		{
			cout << "Started explorer" << endl;
		}
		else
		{
			cout << "Failed to start explorer" << endl;

		}

		PostQuitMessage(0);
		return 0;

	case WM_CLOSE: //this message handling has the same context as WM_DESTROY
		UnhookWindowsHookEx(hook_keys);
		UnhookWindowsHookEx(new_hook_keys);
		if (StartExplorerMinimized())
		{
			cout << "Started explorer" << endl;
		}
		else
		{
			cout << "Failed to start explorer" << endl;

		}

		PostQuitMessage(0);
		return 0;

	default: //default message
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}


LRESULT CALLBACK LLKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	PKBDLLHOOKSTRUCT hookstruct;

	if (nCode == HC_ACTION)
	{
		switch (wParam)
		{
		case WM_KEYDOWN: case WM_SYSKEYDOWN: case WM_SYSCOMMAND:
		case WM_KEYUP: case WM_SYSKEYUP: case WM_DEADCHAR: case WM_SYSCHAR: case WM_HOTKEY:
			return 1;
		}
	}
	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

LRESULT CALLBACK LLMouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	

	if (nCode >= 0)
	{
		switch (wParam)
		{
		case WM_MOUSEMOVE: case WM_LBUTTONUP: case WM_MOUSEWHEEL: case WM_MBUTTONDOWN:
		case WM_LBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDOWN: case WM_MBUTTONUP:
			return 1;
		}

		return CallNextHookEx(new_hook_keys, nCode, wParam, lParam);
	}

}