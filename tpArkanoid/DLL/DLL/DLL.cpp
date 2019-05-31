// MathLibrary.cpp : Defines the exported functions for the DLL.
#include "stdafx.h"
#include <utility>
#include <limits.h>
#include <tchar.h>
#include "DLL.h"

HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hGameDataEvent;
Message *pMessage;
GameData *pGameData;

Message aux;
int inCounter = 0;
int outCounter = 0;

int InitializeClientConnections() {
	/////////////////////SERVER TO CLIENT
	hMapFileStoC = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		MAPPED_FILE_NAME_SC);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	hMutexStoC = CreateMutex(NULL, FALSE, MUTEX_NAME_SC);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("Mutex Error (%d).\n"), GetLastError());
		return 0;
	}

	hSemaphoreSS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_SS);
	if (hSemaphoreSS == NULL) {
		_tprintf(TEXT("Client Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

	hSemaphoreSC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_SC);
	if (hSemaphoreSC == NULL) {
		_tprintf(TEXT("Server Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

//////////////////////////////////CLIENT TO SERVER
	hMapFileCtoS = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		MAPPED_FILE_NAME_CS);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	hMutexCtoS = CreateMutex(NULL, FALSE, MUTEX_NAME_CS);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("Mutex Error (%d).\n"), GetLastError());
		return 0;
	}

	hSemaphoreCC = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_CC);
	if (hSemaphoreSS == NULL) {
		_tprintf(TEXT("Client Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}

	hSemaphoreCS = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, SEMAPHORE_NAME_CS);
	if (hSemaphoreSC == NULL) {
		_tprintf(TEXT("Server Semaphore Error (%d).\n"), GetLastError());
		return 0;
	}
	/////////MAPPING MESSAGE
	pMessage = (Message*)MapViewOfFile(
		hMapFileCtoS,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		MSGBUFFERSIZE * sizeof(Message));
	if (pMessage == NULL) {
		_tprintf(TEXT("Share Memory View Error (%d). \n"), GetLastError());
		CloseHandle(pMessage);
		return 0;
	}

	///////////////////GAMEDATA
	hGameData = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		GAMEDATA_FILE_NAME);
	if (hGameData == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	hGameDataEvent = OpenEvent(
		EVENT_ALL_ACCESS,
		FALSE,
		GAMEDATA_EVENT_FILE_NAME);
	if (hGameDataEvent == NULL) {
		printf("Open Event failed (%d)\n", GetLastError());
		return 0;
	}

	///////////MAP GAMEDATA
	pGameData = (GameData*)MapViewOfFile(
		hGameData,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(GameData));
	if (pGameData == NULL) {
		_tprintf(TEXT("Map View of File Error: (%d). \n"), GetLastError());
		CloseHandle(pGameData);
		return 0;
	}


	return 1;
}

void Login() {
	Message aux;
	TCHAR name[STRINGBUFFERSIZE];

	_tprintf(TEXT("Username --> "));
	fflush(stdin);
	_fgetts(name, STRINGBUFFERSIZE, stdin);
	name[_tcslen(name) - 1] = '\0';

	aux.header = 1;
	_tcscpy_s(aux.content.userName, _countof(aux.content.userName), name);
	SendMessageToServer(aux);
}

int SendMessageToServer(Message content) {

	WaitForSingleObject(hSemaphoreCC, INFINITE);
	WaitForSingleObject(hMutexCtoS, INFINITE);
	*(pMessage + inCounter) = content;
	inCounter = (inCounter + 1) % MSGBUFFERSIZE;
	ReleaseMutex(hMutexCtoS);
	if (!ReleaseSemaphore(hSemaphoreCS, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}

	return 1;
}

int ReceiveMessage(Message* aux) {
	WaitForSingleObject(hSemaphoreSC, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);

	*aux = *(pMessage + outCounter);
	outCounter = (outCounter + 1) % MSGBUFFERSIZE;

	ReleaseMutex(hMutexStoC);
	if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
		return 0;
	}

	return 1;
}

int ReceiveBroadcast(GameData *gameData) {
	WaitForSingleObject(hGameDataEvent, INFINITE);
	*gameData = *pGameData;
	ResetEvent(hGameDataEvent);

	return 1;
}
