#pragma once

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include "common.h"

extern "C" DLL_API int initializeSharedMemory(HANDLE *hMapFileStoC, HANDLE *hMutexStoC, HANDLE *hSemaphoreSS, HANDLE *hSemaphoreSC,
											  HANDLE *hMapFileCtoS, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCC, HANDLE *hSemaphoreCS);

extern "C" DLL_API int sendMessage(Message content, Message *p, HANDLE *hSemaphoreCC, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCS, int *inCounter);

extern "C" DLL_API void login(Message *p, HANDLE *hSemaphoreCC, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCS, int *inCounter);
