// MathLibrary.cpp : Defines the exported functions for the DLL.
#include "stdafx.h"
#include <utility>
#include <limits.h>
#include <tchar.h>
#include "DLL.h"

HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hMutexGameDataShare, hGameDataEvent;
Buffer *pBufferStoC;
Buffer *pBufferCtoS;

HANDLE hPipeMessage, hPipeGameData;

GameData *pGameData;

int myId = -1;

int LocalInitializeClientConnections() {
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

	hMutexStoC = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME_SC);
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

	hMutexGameDataShare = OpenMutex(SYNCHRONIZE, FALSE, MUTEX_NAME_GAMEDATA_SHARE);
	if (hMutexGameDataShare == NULL) {
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

int LocalLogin(TCHAR *name) {
	Message aux;

	aux.header = 1; //Login
	_tcscpy_s(aux.content.userName, _countof(aux.content.userName), name);
	_tprintf(TEXT("%s\n"), aux.content.userName);
	LocalSendMessage(aux);

	WaitForSingleObject(hSemaphoreSC, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);
	aux = pBufferStoC->message[pBufferStoC->out];

	if (aux.header == 2 && _tcscmp(aux.content.userName, name) == 0) { //if rejected and is for me
		pBufferStoC->out = (pBufferStoC->out + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return -1;
		}
		return 0;
	}
	else if (aux.header == 1 && (_tcscmp(aux.content.userName, name) == 0)) {//if accepted and is for me
		myId = aux.id;
		pBufferStoC->out = (pBufferStoC->out + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return 0;
		}
		_tprintf(TEXT("My ID --> %d\n"), myId);
		return 1;
	} else {
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSC, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
			return -1;
		}
		return 0;
	}
	
}

int LocalSendMessage(Message content) {
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

int LocalReceiveMessage(Message *aux) {
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

int LocalReceiveBroadcast(GameData *gameData) {
	WaitForSingleObject(hGameDataEvent, INFINITE);
	WaitForSingleObject(hMutexGameDataShare, INFINITE);
	*gameData = *pGameData;
	ReleaseMutex(hMutexGameDataShare);
	ResetEvent(hGameDataEvent);

	return 1;
}

int PipeInitialize() {
	LPCTSTR pipeName = PIPE_NAME;
	DWORD mode =PIPE_READMODE_MESSAGE;
	BOOL  fSuccess;

	for (int i = 0; i < MAX_CONECTION_TRIES; i++) {
		hPipeMessage = CreateFile(
			pipeName,
			GENERIC_READ | GENERIC_WRITE,
			0,
			NULL,
			OPEN_EXISTING,
			0,
			NULL);

		if (hPipeMessage != INVALID_HANDLE_VALUE)
			break;

		if (GetLastError() != ERROR_PIPE_BUSY) {
			_tprintf(TEXT("Can't open pipe. Error: %d. Tries: %d"), GetLastError(), i);
			return -1;
		}
	}

	mode = PIPE_READMODE_MESSAGE;
	fSuccess = SetNamedPipeHandleState(
		hPipeMessage,
		&mode,
		NULL,
		NULL);
	if (!fSuccess) {
		_tprintf(TEXT("SetNamedPipe Error: %d."), GetLastError());
		return -1;
	}	
	return 1;
}

int RemoteLogin(TCHAR *name) {
	Message aux;
	BOOL fSuccess;
	DWORD bytesRead = 0;
	DWORD mode;
	BOOL isConnected = FALSE;
	TCHAR auxT[STRINGBUFFERSIZE];
	LPCTSTR pipeName;

	aux.header = 1; //Login
	_tcscpy_s(aux.content.userName, _countof(aux.content.userName), name);
	PipeSendMessage(aux);

	fSuccess = ReadFile(
		hPipeMessage,
		&aux,
		sizeof(Message),
		&bytesRead,
		NULL);


	if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
		return -1;

	if (aux.content.confirmation == TRUE) {
		myId = aux.id;
		_stprintf_s(auxT, _countof(auxT), PIPE_NAME_GAMEDATA, myId);
		pipeName = auxT;
		//_tprintf(TEXT("%s\n"), pipeName);

		for (int i = 0; i < LOGIN_RECEIVE_TRIALS; i++) {
			hPipeGameData = CreateFile(
				pipeName,
				GENERIC_READ | GENERIC_WRITE,
				0,
				NULL,
				OPEN_EXISTING,
				0,
				NULL);

			if (hPipeGameData != INVALID_HANDLE_VALUE)
				break;

			if (GetLastError() != ERROR_PIPE_BUSY && i == LOGIN_RECEIVE_TRIALS) {
				_tprintf(TEXT("Can't open pipe. Error: %d.\n"), GetLastError());
				return -1;
			}
			Sleep(500);
		}

		mode = PIPE_READMODE_MESSAGE;
		fSuccess = SetNamedPipeHandleState(
			hPipeGameData,
			&mode,
			NULL,
			NULL);
		if (!fSuccess) {
			_tprintf(TEXT("SetNamedPipe Error: %d."), GetLastError());
			return -1;
		}
		_tprintf(TEXT("GamePipe successfully Created\n"));
		return 1;
	}
	else 
		return 0;

}

int PipeReceiveMessage(Message *aux) {
	BOOL fSuccess;
	DWORD bytesRead = 0;

	fSuccess = ReadFile(
		hPipeMessage,
		aux,
		sizeof(Message),
		&bytesRead,
		NULL);

	if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
		return -1;

	return 1;
}

int PipeSendMessage(Message content) {
	BOOL fSuccess = FALSE;
	DWORD bytesWritten = 0;

	content.id = myId;

	fSuccess = WriteFile(
		hPipeMessage,
		&content,
		sizeof(Message),
		&bytesWritten,
		NULL);

	if (!fSuccess || bytesWritten != sizeof(Message)) {
		_tprintf(TEXT("WriteFile failed. Error: %d\n"), GetLastError());
		return -1;
	}

	return 1;
}

int RemoteReceiveGameData(GameData *gameData) {
	BOOL fSuccess;
	DWORD bytesRead = 0;

	fSuccess = ReadFile(
		hPipeGameData,
		gameData,
		sizeof(GameData),
		&bytesRead,
		NULL);

	if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
		return -1;
	if (bytesRead != sizeof(GameData))
		return -1;

	return 1;
}

