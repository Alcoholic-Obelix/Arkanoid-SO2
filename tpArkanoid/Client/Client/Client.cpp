// Client.cpp : This file contains the 'main' function. Program execution begins and ends there.
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
#include "common.h"
#include "DLL.h"

HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hGameDataEvent;
int inCounter = 0;
int outCounter = 0;
GameData gameData;

void gotoxy(int x, int y) {
	static HANDLE hStdout = NULL;
	COORD coord;
	coord.X = x;
	coord.Y = y;
	if (!hStdout)
		hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleCursorPosition(hStdout, coord);
}

void clearScreen() {
	for (int i = 0; i <= MAX_HEIGHT; i++) {
		for (int j = 0; j <= MAX_WIDTH; j++) {
			gotoxy(j, i);
			_tprintf(TEXT(" "));
		}
	}
	gotoxy(0, 0);
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

void drawBall(int x, int y, int previousX, int previousY) {
		gotoxy(previousX, previousY);
		_tprintf(TEXT(" "));
		gotoxy(x, y);
		_tprintf(TEXT("O"));
}

DWORD WINAPI ReadMessages(LPVOID param) {
	Message *p;
	Message aux;
	int previousX = 1, previousY = 1;

	p = (Message*)MapViewOfFile(
		hMapFileStoC,
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
		WaitForSingleObject(hSemaphoreSC, INFINITE);
		WaitForSingleObject(hMutexStoC, INFINITE);
		aux = *(p + outCounter);
		outCounter = (outCounter + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexStoC);
		if (!ReleaseSemaphore(hSemaphoreSS, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
		}

		switch (aux.header) {
			case 2:
				if (aux.content.confirmation == true) {
					drawBorders();
				}
		}
	}
}

int initialization() {
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
	return 1;
}

DWORD WINAPI UpdateGameData(LPVOID param) {
	GameData *p;
	int previousX = 1, previousY = 1;

	p = (GameData*)MapViewOfFile(
		hGameData,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(GameData));
	if (p == NULL) {
		_tprintf(TEXT("Map View of File Error: (%d). \n"), GetLastError());
		CloseHandle(p);
		return 0;
	}

	gameData.gameState = GAME;
	while (gameData.gameState == GAME) {
		WaitForSingleObject(hGameDataEvent, INFINITE);
		gameData = *p;
		drawBall(gameData.ball.x, gameData.ball.y, previousX, previousY);
		previousX = gameData.ball.x;
		previousY = gameData.ball.y;
	}
	return 1;
}

int _tmain(int argc, LPTSTR argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	DWORD readMessagesThreadId, updateGameDataThreadID;
	Message *p;
	Message aux;

	if (initializeSharedMemory(&hMapFileStoC, &hMutexStoC, &hSemaphoreSS, &hSemaphoreSC,
							   &hMapFileCtoS, &hMutexCtoS, &hSemaphoreCC, &hSemaphoreCS) == 0) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}
	initialization();

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

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadMessages, NULL, 0, &readMessagesThreadId);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UpdateGameData, NULL, 0, &updateGameDataThreadID);

	login(p, &hSemaphoreCC, &hMutexCtoS, &hSemaphoreCS, &inCounter);
	drawBorders();




	_gettch();
	aux.header = 4;
	sendMessage(aux, p, &hSemaphoreCC, &hMutexCtoS, &hSemaphoreCS, &inCounter);
	clearScreen();
	Sleep(500);

	TerminateThread(&readMessagesThreadId, 0);
	CloseHandle(hMapFileStoC);

}

