// MathLibrary.cpp : Defines the exported functions for the DLL.
#include "stdafx.h"
#include <utility>
#include <limits.h>
#include <tchar.h>
#include "DLL.h"

HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hMutexGameData, hGameDataEvent;

Buffer *pBufferStoC;
Buffer *pBufferCtoS;
GameData *pGameData;

Message aux;
int myId = -1;


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

	pBufferStoC = (Buffer*)MapViewOfFile(
		hMapFileStoC,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(Buffer));
	if (pBufferStoC == NULL) {
		_tprintf(TEXT("Share Memory View Error (%d). \n"), GetLastError());
		CloseHandle(pBufferStoC);
		return 0;
	}

	hMutexStoC = OpenMutex(SYNCHRONIZE, false, MUTEX_NAME_SC);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("Mutex Opening Error (%d).\n"), GetLastError());
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

	pBufferCtoS = (Buffer*)MapViewOfFile(
		hMapFileCtoS,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(Buffer));
	if (pBufferCtoS == NULL) {
		_tprintf(TEXT("Share Memory View Error (%d). \n"), GetLastError());
		CloseHandle(pBufferCtoS);
		return 0;
	}

	hMutexCtoS = OpenMutex(SYNCHRONIZE, false, MUTEX_NAME_CS);
	if (hMutexCtoS == NULL) {
		_tprintf(TEXT("Mutex Opening Error (%d).\n"), GetLastError());
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

	///////////////////GAMEDATA
	hGameData = OpenFileMapping(
		FILE_MAP_ALL_ACCESS,
		FALSE,
		GAMEDATA_FILE_NAME);
	if (hGameData == NULL) {
		_tprintf(TEXT("Error opening file: (%d).\n"), GetLastError());
		return 0;
	}

	hMutexGameData = OpenMutex(SYNCHRONIZE, false, MUTEX_NAME_GAMEDATA);
	if (hMutexGameData == NULL) {
		_tprintf(TEXT("Mutex Opening Error (%d).\n"), GetLastError());
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

int Login(TCHAR *name) {
	Message aux;

	aux.header = 1; //Login
	_tcscpy_s(aux.content.userName, _countof(aux.content.userName), name);
	SendMessageToServer(aux);

	WaitForSingleObject(hSemaphoreSC, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);
	if (_tcscmp(pBufferStoC->message[pBufferStoC->out].content.userName, name) == 0) {
		myId = pBufferStoC->message[pBufferStoC->out].id;
		pBufferStoC->out = (pBufferStoC->out + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return 0;
		}
		_tprintf(TEXT("%d\n"), myId);
		return 1;
	}
	else {
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSC, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return 0;
		}
		return 0;
	}

}

int SendMessageToServer(Message content) {
	WaitForSingleObject(hSemaphoreCC, INFINITE);
	WaitForSingleObject(hMutexCtoS, INFINITE);
	content.id = myId;
	pBufferCtoS->message[pBufferCtoS->in] = content;
	pBufferCtoS->in = (pBufferCtoS->in + 1) % MSGBUFFERSIZE;
	ReleaseMutex(hMutexCtoS);
	if (!ReleaseSemaphore(hSemaphoreCS, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}

	return 1;
}

int ReceiveMessage(Message* aux) {
	WaitForSingleObject(hSemaphoreSC, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);

	if (pBufferStoC->message[pBufferStoC->out].id == myId) {
		*aux = pBufferStoC->message[pBufferStoC->out];
		pBufferStoC->out = (pBufferStoC->out + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return 0;
		}
	}
	else {
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSC, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return 0;
		}
	}

	return 1;
}

int ReceiveBroadcast(GameData *gameData) {
	WaitForSingleObject(hGameDataEvent, INFINITE);
	WaitForSingleObject(hMutexGameData, INFINITE);
	*gameData = *pGameData;
	ReleaseMutex(hMutexGameData);
	ResetEvent(hGameDataEvent);

	return 1;
}
