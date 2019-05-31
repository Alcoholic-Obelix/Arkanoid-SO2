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

DWORD WINAPI ReadMessages(LPVOID param) {
	Message aux;
	int previousX = 1, previousY = 1;

	while (1) {
		if (ReceiveMessage(&aux) == 0) {					//receives message from server through the DLL
			_tprintf(TEXT("Message couldn't be read \n"));	//and controls the semaphores and mutexes
			return 0;
		}
		else {
			_tprintf(TEXT("Message read \n"));
		}
		
		switch (aux.header) {
			case 2:
				if (aux.content.confirmation == true) {
					drawBorders();
				}
		}
	}
}

DWORD WINAPI UpdateGameData(LPVOID param) {
	int previousX = 1, previousY = 1;	

	while (1) {
		ReceiveBroadcast(&gameData);
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
	DWORD readMessagesThreadId, updateGameDataThreadID;
	Message aux;
	
	InitializeClientConnections();

	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadMessages, NULL, 0, &readMessagesThreadId);
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)UpdateGameData, NULL, 0, &updateGameDataThreadID);

	Login();
	drawBorders();




	_gettch();
	aux.header = 4;
	SendMessageToServer(aux);
	clearScreen();
	Sleep(500);

	TerminateThread(&readMessagesThreadId, 0);
}

