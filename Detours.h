#pragma once

#include <detours.h>

#define DETOUR(original, hook)\
	DetourTransactionBegin();\
	DetourUpdateThread(GetCurrentThread());\
	DetourAttach(&(PVOID&)original, hook);\
	DetourTransactionCommit();\

typedef unsigned int (*_PollStatus)(__int64 a1, int a2, char a3, char a4);
typedef void* (*_GetClientInterface)(__int64 a1);

inline unsigned int PollStatusDetour(__int64 a1, int a2, char a3, char a4)
{
	return 1;
}

inline void* GetClientInterfaceDetour(__int64 a1)
{
	return (void*)1;
}