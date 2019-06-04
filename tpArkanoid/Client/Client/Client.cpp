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


GameData gameData;
TCHAR myName[STRINGBUFFERSIZE];
BOOL isLocal;
int myId;

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

int askForLogin() {

	_tprintf(TEXT("Username --> "));
	fflush(stdin);
	_fgetts(myName, STRINGBUFFERSIZE, stdin);
	myName[_tcslen(myName) - 1] = '\0';

	if (isLocal) {
		for (int i = 0; i < LOGIN_TRIALS; i++) {
			if (LocalLogin(myName) == 1) {
				_tprintf(TEXT("Logged In Locally\n"));
				return 1;
			}				
		}
	}		
	else
		if (RemoteLogin(myName) == 1) {
			_tprintf(TEXT("Logged In Remotelly\n"));
			return 1;
		}

	return 0;
}

DWORD WINAPI ReadPipedMessages(LPVOID param) {
	Message aux;

	while (gameData.gameState != OFF) {
		if (PipeReceiveMessage(&aux) == 0) {					//receives message from server through the DLL
			_tprintf(TEXT("PipeMessage couldn't be read \n"));	//and controls the semaphores and mutexes
			return -1;
		} else {
			_tprintf(TEXT("Message read \n"));
		}

		switch (aux.header) {
			case 2:
			break;
		}
	}	
}

DWORD WINAPI ReadLocalMessages(LPVOID param) {
	Message aux;
	int previousX = 1, previousY = 1;

	while (gameData.gameState != OFF) {
		if (LocalReceiveMessage(&aux) == 0) {					//receives message from server through the DLL
			_tprintf(TEXT("Message couldn't be read \n"));	//and controls the semaphores and mutexes
			return 0;
		}
		else {
			_tprintf(TEXT("Message read \n"));
		}
		
		switch (aux.header) {
			case 1:
				if (_tcscmp(myName, aux.content.userName) == 0) {
					myId = aux.id;
				}
		}
	}
}

DWORD WINAPI LocalUpdateGameData(LPVOID param) {
	int previousX = 1, previousY = 1;	

	while (gameData.gameState != OFF) {
		LocalReceiveBroadcast(&gameData);
		drawBall(gameData.balls[0].x, gameData.balls[0].y, previousX, previousY);
		previousX = gameData.balls[0].x;
		previousY = gameData.balls[0].y;
	}
	return 1;
}

DWORD WINAPI RemoteUpdateGameData(LPVOID param) {
	int previousX = 1, previousY = 1;

	while (gameData.gameState != OFF) {
		RemoteReceiveGameData(&gameData);
		drawBall(gameData.balls[0].x, gameData.balls[0].y, previousX, previousY);
		previousX = gameData.balls[0].x;
		previousY = gameData.balls[0].y;
	}
	return 1;

}

int _tmain(int argc, LPTSTR argv[]) {
#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif
	HANDLE hReadMessagesThread, hUpdateGameDataThread;
	Message aux;

	
	isLocal = FALSE;
	gameData.gameState = LOGIN;

	if (isLocal) {
		LocalInitializeClientConnections();
	}
	else		
		PipeInitialize();


	if (askForLogin() == 1) {
		gameData.gameState = GAME;
		if (isLocal) {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadLocalMessages, NULL, 0, NULL);
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LocalUpdateGameData, NULL, 0, NULL);
		} else {
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipedMessages, NULL, 0, NULL);
			CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RemoteUpdateGameData, NULL, 0, NULL);
		}
	}
	else {
		_tprintf(TEXT("Login Error\n"));
		return 0;
	}
	Sleep(500);
	drawBorders();
	//clearScreen();

	_gettch();
}

