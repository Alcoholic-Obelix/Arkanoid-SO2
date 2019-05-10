// Server.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include <iostream>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <tchar.h>
#include <crtdefs.h>
#include <process.h>
#include <malloc.h>
#include "Auxiliary.h"
#include "..\..\Client\Client\common.h"


HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hGameDataEvent;
GameData gameData;

int inCounter = 0;
int outCounter = 0;

int initialization() {
	//Game Data
	hGameData = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		sizeof(GameData),
		GAMEDATA_FILE_NAME);
	if (hGameData == NULL) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}

	hGameDataEvent = CreateEvent(
		NULL,               
		TRUE,               // manual-reset event
		FALSE,              // initial state is nonsignaled
		GAMEDATA_EVENT_FILE_NAME  // object name
	);
	if (hGameDataEvent == NULL) {
		printf("CreateEvent failed (%d)\n", GetLastError());
		return 0;
	}

	//ServerToClient
	hMapFileStoC = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		10 * sizeof(Message),
		MAPPED_FILE_NAME_SC);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}

	hMutexStoC = CreateMutex(NULL, FALSE, MUTEX_NAME_SC);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("O Mutex deu problemas (%d).\n"), GetLastError());
		Sleep(500);
		return 0;
	}

	hSemaphoreSS = CreateSemaphore(
		NULL,
		MAX_SEM_COUNT,
		MAX_SEM_COUNT,
		SEMAPHORE_NAME_SS);
	if (hSemaphoreSS == NULL) {
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 0;
	}

	hSemaphoreSC = CreateSemaphore(
		NULL,
		MIN_SEM_COUNT,
		MAX_SEM_COUNT,
		SEMAPHORE_NAME_SC);
	if (hSemaphoreSC == NULL) {
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 0;
	}

	//ClientToServer
	hMapFileCtoS = CreateFileMapping(
		INVALID_HANDLE_VALUE,
		NULL,
		PAGE_READWRITE,
		0,
		10 * sizeof(Message),
		MAPPED_FILE_NAME_CS);
	if (hMapFileStoC == NULL) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}

	hMutexCtoS = CreateMutex(NULL, FALSE, MUTEX_NAME_CS);
	if (hMutexCtoS == NULL) {
		_tprintf(TEXT("O Mutex deu problemas (%d).\n"), GetLastError());
		Sleep(500);
		return 0;
	}

	hSemaphoreCC = CreateSemaphore(
		NULL,
		MAX_SEM_COUNT,
		MAX_SEM_COUNT,
		SEMAPHORE_NAME_CC);
	if (hSemaphoreSS == NULL) {
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 0;
	}

	hSemaphoreCS = CreateSemaphore(
		NULL,
		MIN_SEM_COUNT,
		MAX_SEM_COUNT,
		SEMAPHORE_NAME_CS);
	if (hSemaphoreSC == NULL) {
		printf("CreateSemaphore error: %d\n", GetLastError());
		return 0;
	}
	return 1;
}

void drawBorders() {
	for (int i = 0; i < MAX_HEIGHT; i++) {
		for (int j = 0; j < MAX_WIDTH; j++) {
			gotoxy(j, i);
			_tprintf(TEXT(" "));
		}
	}
	for (int i = 0; i < MAX_WIDTH; i++) {
		gotoxy(i, 0);
		_tprintf(TEXT("#"));
	}
	for (int i = 1; i < MAX_HEIGHT; i++) {
		gotoxy(0, i);
		_tprintf(TEXT("#"));
		gotoxy(MAX_WIDTH - 1, i);
		_tprintf(TEXT("#"));
	}
	for (int i = 0; i < MAX_WIDTH; i++) {
		gotoxy(i, MAX_HEIGHT);
		_tprintf(TEXT("#"));
	}
}

void sendMessage(Message content, int* inCounter) {
	Message *p;

	p = (Message*)MapViewOfFile(
		hMapFileStoC,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		MSGBUFFERSIZE * sizeof(Message));
	if (p == NULL) {
		_tprintf(TEXT("Share Memory View Error (%d). \n"), GetLastError());
		CloseHandle(p);
	}

	WaitForSingleObject(hSemaphoreSS, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);
	*(p + *inCounter) = content;
	*inCounter = ((*inCounter) + 1) % MSGBUFFERSIZE;
	ReleaseMutex(hMutexStoC);
	if (!ReleaseSemaphore(hSemaphoreSC, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}	
}

void updateGameData() {
	GameData *p;

	p = (GameData*)MapViewOfFile(
		hGameData,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(GameData));
	if (p == NULL) {
		_tprintf(TEXT("Map View of File Error: (%d). \n"), GetLastError());
		CloseHandle(p);
	}

	*p = gameData;

	if (!SetEvent(hGameDataEvent)) {
		printf("SetEvent failed (%d)\n", GetLastError());
		return;
	}
}

DWORD WINAPI BallThread(LPVOID param) {
	int x, y, xSpeed, ySpeed;
	xSpeed = 1;
	ySpeed = 1;
	x = 1;
	y = 1;

	while (gameData.gameState == GAME) {
		gameData.ball.x = x;
		gameData.ball.y = y;
		updateGameData();
		Sleep(100);
		x = x + xSpeed;
		y = y + ySpeed;

		if (x <= 1 || x >= MAX_WIDTH - 2)
			xSpeed *= -1;
		if (y <= 1 || y >= MAX_HEIGHT - 1)
			ySpeed *= -1;
	}
	_tprintf(TEXT("Bye Ball\n"));
	return 0;
}

DWORD WINAPI ReadMessages(LPVOID param) {
	Message *p;
	Message aux;
	DWORD ballThreadId;

	p = (Message*)MapViewOfFile(
		hMapFileCtoS,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		MSGBUFFERSIZE * sizeof(Message));
	if (p == NULL) {
		_tprintf(TEXT("Share Memory View Error (%d). \n"), GetLastError());
		CloseHandle(p);
		return 0;
	}
	while (1) {
		WaitForSingleObject(hSemaphoreCS, INFINITE);
		WaitForSingleObject(hMutexCtoS, INFINITE);
		aux = *(p + outCounter);
		outCounter = (outCounter + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexCtoS);
		if (!ReleaseSemaphore(hSemaphoreCC, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
		}

		switch (aux.header) {
			case 1 :
				_tcscpy_s(gameData.userName, _countof(gameData.userName), aux.content.userName);
				gameData.gameState = GAME;
				aux.header = 2;
				aux.content.confirmation = true;
				sendMessage(aux, &inCounter);
				_tprintf(TEXT("%s is logged in. The game will start soon.\n"), gameData.userName);
				Sleep(1000);
				CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BallThread, NULL, 0, &ballThreadId);
				break;
			case 4:
				gameData.gameState = 4;
				updateGameData();
				break;
		}
	}
	_tprintf(TEXT("Bye Messages\n"));
}

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	HKEY key;
	DWORD disposition, score;
	TCHAR command[STRINGBUFFERSIZE];
	DWORD readMessagesThreadId;

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\InvadersTop10"), 0, NULL,
					   REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &key, &disposition) != ERROR_SUCCESS) {
		_tprintf(TEXT("Erro ao criar/abrir chave (%d)\n"), GetLastError());
		return -1;
	}

	if (initialization() == 0) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}

	gameData.gameState = LOGIN;

	while (gameData.gameState != OFF) {
		fflush(stdin);
		_fgetts(command, STRINGBUFFERSIZE, stdin);
		command[_tcslen(command) - 1] = '\0';
		//_tprintf(command);

		if (_tcscmp(command, TEXT("start")) == 0) {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadMessages, NULL, 0, &readMessagesThreadId);
			_tprintf(TEXT("Server Ready\n"));
		}
		else if (_tcscmp(command, TEXT("top")) == 0) {
			gameData.score = 115;
			score = gameData.score;
			RegSetValueEx(key, TEXT("Name"), 0, REG_SZ, (LPBYTE)TEXT("Ricardo"), _tcslen(TEXT("Ricardo")) * sizeof(TCHAR));
			RegSetValueEx(key, TEXT("Score"), 0, REG_DWORD, (LPBYTE)&score, sizeof(DWORD));
		}
		else if (_tcscmp(command, TEXT("exit")) == 0) {
			TerminateThread(&readMessagesThreadId, 0);
			CloseHandle(hMapFileStoC);
			gameData.gameState = OFF;
		}
	}
}



