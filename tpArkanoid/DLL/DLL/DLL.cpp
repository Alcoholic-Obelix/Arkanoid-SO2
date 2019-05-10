// MathLibrary.cpp : Defines the exported functions for the DLL.
#include "stdafx.h"
#include <utility>
#include <limits.h>
#include <tchar.h>
#include "DLL.h"

int initializeSharedMemory(HANDLE *hMapFileStoC, HANDLE *hMutexStoC, HANDLE *hSemaphoreSS, HANDLE *hSemaphoreSC, 
						   HANDLE *hMapFileCtoS, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCC, HANDLE *hSemaphoreCS) {
	/////////////////////SERVER TO CLIENT
	*hMapFileStoC = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		MAPPED_FILE_NAME_SC);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	*hMutexStoC = CreateMutex(NULL, FALSE, MUTEX_NAME_SC);
	if (*hMutexStoC == NULL) {
		_tprintf(TEXT("Mutex Error (%d).\n"), GetLastError());
		return 0;
	}

	*hSemaphoreSS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_SS);
	if (*hSemaphoreSS == NULL) {
		_tprintf(TEXT("Client Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

	*hSemaphoreSC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_SC);
	if (*hSemaphoreSC == NULL) {
		_tprintf(TEXT("Server Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

//////////////////////////////////CLIENT TO SERVER
	*hMapFileCtoS = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		MAPPED_FILE_NAME_CS);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	*hMutexCtoS = CreateMutex(NULL, FALSE, MUTEX_NAME_CS);
	if (*hMutexStoC == NULL) {
		_tprintf(TEXT("Mutex Error (%d).\n"), GetLastError());
		return 0;
	}

	*hSemaphoreCC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_CC);
	if (*hSemaphoreSS == NULL) {
		_tprintf(TEXT("Client Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

	*hSemaphoreCS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_CS);
	if (*hSemaphoreSC == NULL) {
		_tprintf(TEXT("Server Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}


	return 1;
}

int sendMessage(Message content, Message *p, HANDLE *hSemaphoreCC, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCS, int *inCounter) {

	WaitForSingleObject(*hSemaphoreCC, INFINITE);
	WaitForSingleObject(*hMutexCtoS, INFINITE);
	*(p + *(inCounter)) = content;
	*inCounter = ((*inCounter) + 1) % MSGBUFFERSIZE;
	ReleaseMutex(*hMutexCtoS);
	if (!ReleaseSemaphore(*hSemaphoreCS, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}

	return 1;
}

void login(Message *p, HANDLE *hSemaphoreCC, HANDLE *hMutexCtoS, HANDLE *hSemaphoreCS, int *inCounter) {
	Message aux;
	TCHAR name[STRINGBUFFERSIZE];

	_tprintf(TEXT("Username --> "));
	fflush(stdin);
	_fgetts(name, STRINGBUFFERSIZE, stdin);
	name[_tcslen(name) - 1] = '\0';

	aux.header = 1;
	_tcscpy_s(aux.content.userName, _countof(aux.content.userName), name);
	sendMessage(aux, p, hSemaphoreCC, hMutexCtoS, hSemaphoreCS, inCounter);
}