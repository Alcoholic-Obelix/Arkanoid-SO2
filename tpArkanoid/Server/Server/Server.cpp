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
#include "..\..\DLL\DLL\common.h"


HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hGameDataEvent;
GameData gameData;
Config config;
ClientsInfo *clientsInfo;


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

int configSwitch(TCHAR *str, int value) {
	//"switch case" de strings para comparar no ficheiro config. Se corresponder adiciona o value ao campo correspondente
	if (_tcscmp(str, TEXT("max_players")) == 0) {
		config.maxPlayers = value;
	}
	else if(_tcscmp(str, TEXT("levels")) == 0) {
		config.levels = value;
	}
	else if (_tcscmp(str, TEXT("lives")) == 0) {
		config.nLives = value;
	}
	else if (_tcscmp(str, TEXT("ball_speed")) == 0) {
		config.ballSpeed = value;
	}
	else if (_tcscmp(str, TEXT("bricks")) == 0) {
		config.nBricks = value;
	}
	else if (_tcscmp(str, TEXT("powerup_speed")) == 0) {
		config.powerUpSpeed = value;
	}
	else if (_tcscmp(str, TEXT("speedups")) == 0) {
		config.nSpeedUps = value;
	}
	else if (_tcscmp(str, TEXT("speed_up_duration")) == 0) {
		config.speedUpDuration = value;
	}
	else if (_tcscmp(str, TEXT("slowdowns")) == 0) {
		config.nSlowDowns = value;
	}
	else if (_tcscmp(str, TEXT("slowdown_duration")) == 0) {
		config.slowDownDuration = value;
	}
	else if (_tcscmp(str, TEXT("triples")) == 0) {
		config.nTriples = value;
	}
	else if (_tcscmp(str, TEXT("triple_duration")) == 0) {
		config.tripleDuration = value;
	}
	else {
		_tprintf(TEXT("This configuration is not valid: %s\n"), str);
		return 0;
	}

	return 1;
}

int initializeConfig() {

	FILE *f;
	TCHAR line[STRINGBUFFERSIZE];
	TCHAR *tokenName, *tokenValue, *nextToken1, *nextToken2;
	int value;

	_tfopen_s(&f, CONFIGURATION_FILE, TEXT("r"));	//open configuration file

	while (_tfgets(line, STRINGBUFFERSIZE, f) != NULL) {		//reads file line to string 'line'
		tokenName = _tcstok_s(line, TEXT(" "), &nextToken1);	//gets name "command"
		tokenValue = _tcstok_s(nextToken1, TEXT("\n"), &nextToken2);	//gets value in string format
		if (tokenValue[0] != '\n') {
			value = _ttoi(tokenValue);
			configSwitch(tokenName, value);
		}


	}

	return 1;
}

void initializeClientInfo() {
	clientsInfo = (ClientsInfo*) malloc(config.maxPlayers * sizeof(ClientsInfo));

	for (int i = 0; i < config.maxPlayers; i++)	{
		clientsInfo[i].state = EMPTY;
	}
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

	TCHAR command[STRINGBUFFERSIZE];
	DWORD readMessagesThreadId;

	HKEY hKey;
	DWORD result;
	DWORD keyType = REG_BINARY;
	DWORD bufSize = sizeof(Top10);
	Top10 top10, testeResultado;
	

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\ArkanoidTop10"), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &result) != ERROR_SUCCESS) {
		return -1;
	}

	if (initialization() == 0) {
		_tprintf(TEXT("Shared Memory error (%d).\n"), GetLastError());
		return 0;
	}
	if (initializeConfig()== 0) {
		_tprintf(TEXT("Configuration file error\n"));
		return 0;
	}

	initializeClientInfo();

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
			top10.score[0] = gameData.score;
			gameData.score = 245;
			top10.score[1] = gameData.score;
			RegSetValueEx(hKey, TEXT("Top10"), 0, REG_BINARY, (LPBYTE)&top10, sizeof(Top10));
			RegQueryValueEx(hKey, TEXT("Top10"), 0, &keyType, (LPBYTE)&testeResultado, &bufSize);

			_tprintf(TEXT("%d, %d"), testeResultado.score[0], testeResultado.score[1]);
		}
		else if (_tcscmp(command, TEXT("exit")) == 0) {
			TerminateThread(&readMessagesThreadId, 0);
			CloseHandle(hMapFileStoC);
			gameData.gameState = OFF;
		}
	}
}



