#include "pch.h"

#include <iostream>
#include "detours.h"

void WINAPI Main()
{
	auto PollStatusAddr =
		GetProcAddress(
			GetModuleHandle(L"EOSSDK-Win64-Shipping.dll"),
			"EOS_AntiCheatClient_PollStatus");

	_PollStatus PollStatus = (_PollStatus)PollStatusAddr;

	DETOUR(PollStatus, PollStatusDetour);

	auto GetClientInterfaceAddr =
		GetProcAddress(
			GetModuleHandle(L"EOSSDK-Win64-Shipping.dll"),
			"EOS_Platform_GetAntiCheatClientInterface");

	auto GetClientInterface = (_GetClientInterface)GetClientInterfaceAddr;

	DETOUR(GetClientInterface, GetClientInterfaceDetour);
}

BOOL APIENTRY DllMain(
	HMODULE hModule,
	DWORD  ul_reason_for_call,
	LPVOID lpReserved
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		CloseHandle(CreateThread(nullptr, 0, (LPTHREAD_START_ROUTINE)Main, hModule, 0, nullptr));
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

