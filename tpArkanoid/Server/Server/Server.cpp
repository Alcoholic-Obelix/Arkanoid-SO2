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

//SYNCRONITAZION HANDLES
HANDLE hMapFileStoC, hMutexStoC, hSemaphoreSS, hSemaphoreSC;
HANDLE hMapFileCtoS, hMutexCtoS, hSemaphoreCC, hSemaphoreCS;
HANDLE hGameData, hMutexGameDataShare, hMutexGameDataServer, hGameDataEvent;
HANDLE hMutexAddPlayer;
HANDLE hBallTimer;


//FILEMAPPING POINTERS
Buffer *pBufferStoC;
Buffer *pBufferCtoS;
GameData *pGameData;

//GAME VARIABLES
GameData gameData;
Config config;
ClientsInfo clientsInfo[20];
int nClients;
int ballId;
int nBalls;

//TODO: config from main argument

int initializeLocalMemory() {
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

	pGameData = (GameData*)MapViewOfFile(
		hGameData,
		FILE_MAP_ALL_ACCESS,
		0,
		0,
		sizeof(GameData));
	if (pGameData == NULL) {
		_tprintf(TEXT("Map View of File Error: (%d). \n"), GetLastError());
		CloseHandle(pGameData);
	}

	hMutexGameDataServer = CreateMutex(NULL, FALSE, NULL);
	if (hMutexGameDataServer == NULL) {
		_tprintf(TEXT("O Mutex deu problemas (%d).\n"), GetLastError());
		return 0;
	}
	
	hMutexGameDataShare = CreateMutex(NULL, FALSE, MUTEX_NAME_GAMEDATA_SHARE);
	if (hMutexGameDataShare == NULL) {
		_tprintf(TEXT("O Mutex deu problemas (%d).\n"), GetLastError());
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

	pBufferStoC->in = 0;
	pBufferStoC->out = 0;

	hMutexStoC = CreateMutex(NULL, FALSE, MUTEX_NAME_SC);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("O Mutex deu problemas (%d).\n"), GetLastError());
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

	pBufferCtoS->in = 0;
	pBufferCtoS->out = 0;

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

	hBallTimer = CreateWaitableTimer(NULL, TRUE, NULL);
	if (NULL == hBallTimer) 
		printf("CreateWaitableTimer failed (%d)\n", GetLastError());

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

	fclose(f);

	return 1;
}

void initializeClientInfo() {
	hMutexAddPlayer = CreateMutex(NULL, FALSE, MUTEX_NAME_SC);
	if (hMutexStoC == NULL) {
		_tprintf(TEXT("Error in mutex AddPlayer (%d).\n"), GetLastError());
		return;
	}

	for (int i = 0; i < config.maxPlayers; i++)	{
		clientsInfo[i].state = EMPTY;
		gameData.players[i].status = EMPTY;
		gameData.players[i].score = 0;
		gameData.players[i].platform.mode = 1;
		gameData.players[i].platform.x = PLATFORM_START_X;
		gameData.players[i].platform.y = PLATFORM_START_Y;
	}
}


void printClientState() {
	for (int i = 0; i < config.maxPlayers; i++)	{
		_tprintf(TEXT("[%d] -> %d\n"), i, clientsInfo[i].state);
	}
}

int ExitReadingThread() {

	Message exitMessage;
	exitMessage.header = 0;

	WaitForSingleObject(hSemaphoreCC, INFINITE);
	WaitForSingleObject(hMutexCtoS, INFINITE);
	pBufferCtoS->message[pBufferCtoS->in] = exitMessage;
	//*(pMessageCtoS + inCounter) = exitMessage;
	pBufferCtoS->in = (pBufferCtoS->in + 1) % MSGBUFFERSIZE;
	ReleaseMutex(hMutexCtoS);
	if (!ReleaseSemaphore(hSemaphoreCS, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}

	return 1;
}

void sendGameDataToAllPipes() {
	BOOL fSuccess = FALSE;
	DWORD bytesWritten = 0;

		for (int i = 0; i < nClients; i++) {
			if (clientsInfo[i].state == LOGGED_IN && clientsInfo[i].isLocal == FALSE) {
				fSuccess = WriteFile(
				clientsInfo[i].hGamePipe,
				&gameData,
				sizeof(GameData),
				&bytesWritten,
				NULL);
			if (!fSuccess || bytesWritten != sizeof(gameData)) {
				_tprintf(TEXT("GameData WriteFile failed for id: %d. Error: %d\n"), i, GetLastError());
			}
		}
			
	}
}

void updateGameData() {
	WaitForSingleObject(hMutexGameDataShare, INFINITE);
	*pGameData = gameData;
	sendGameDataToAllPipes();
	ReleaseMutex(hMutexGameDataShare);

	if (!SetEvent(hGameDataEvent)) {
		printf("SetEvent failed (%d)\n", GetLastError());
	}
}

void setUpBricks1() {
	int i = 0, aux = 0;
	
	while (i < 10) {
		gameData.bricks[i].y = 30;
		gameData.bricks[i].x = 200 + aux;
		gameData.bricks[i].type = 1;
		gameData.bricks[i].hp = 1;
		aux += BRICK_SIZE_X;
		i++;
	}

	aux = 0;
	while (i < 20) {
		gameData.bricks[i].y = 80;
		gameData.bricks[i].x = 200 + aux;
		gameData.bricks[i].type = 2;
		gameData.bricks[i].hp = 2;
		aux += BRICK_SIZE_X;
		i++;
	}

	aux = 0;
	while (i < 30) {
		gameData.bricks[i].y = 130;
		gameData.bricks[i].x = 200 + aux;
		gameData.bricks[i].type = 1;
		gameData.bricks[i].hp = 1;
		aux += BRICK_SIZE_X;
		i++;
	}
	updateGameData();
}

void sendMessage(Message content) {
	WaitForSingleObject(hSemaphoreSS, INFINITE);
	WaitForSingleObject(hMutexStoC, INFINITE);
	pBufferStoC->message[pBufferStoC->in] = content;
	pBufferStoC->in = ((pBufferStoC->in) + 1) % MSGBUFFERSIZE;
	ReleaseMutex(hMutexStoC);
	if (!ReleaseSemaphore(hSemaphoreSC, 1, NULL)) {
		_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
	}	
}

void sendPipedMessageByHandle(HANDLE hPipe, Message aux) {
	BOOL fSuccess = FALSE;
	DWORD bytesWritten = 0;

	fSuccess = WriteFile(
		hPipe,
		&aux,
		sizeof(Message),
		&bytesWritten,
		NULL);

	if (!fSuccess || bytesWritten != sizeof(Message)) {
		_tprintf(TEXT("WriteFile failed. Error: %d\n"), GetLastError());
	}
}

void sendPipedMessageById(int clientId, Message aux) {
	HANDLE hPipe = clientsInfo[clientId].hMessagePipe;
	BOOL fSuccess = FALSE;
	DWORD bytesWritten = 0;

	fSuccess = WriteFile(
			hPipe,
			&aux,
			sizeof(Message),
			&bytesWritten,
			NULL);

		if (!fSuccess || bytesWritten != sizeof(Message)) {
			_tprintf(TEXT("WriteFile failed. Error: %d\n"), GetLastError());
		}
}

int addUser(BOOL isLocal, TCHAR *name, HANDLE hMessagePipe, HANDLE hGamePipe) {
	int clientId = -1;

	WaitForSingleObject(hMutexAddPlayer, INFINITE);

	for (int i = 0; i <= nClients; i++) {
		if (clientsInfo[i].state == EMPTY) {
			clientId = i;
			break;
		}
	}

	if (clientId == -1) {
		ReleaseMutex(hMutexAddPlayer);
		return -1;
	}
	clientsInfo[clientId].isLocal = isLocal;
	clientsInfo[clientId].state = LOGGED_IN;
	clientsInfo[clientId].hMessagePipe = hMessagePipe;
	clientsInfo[clientId].hGamePipe = hGamePipe;
	_tcscpy_s(clientsInfo[clientId].name, _countof(clientsInfo[clientId].name), name);
	gameData.players[clientId].status = LOGGED_IN;
	nClients++;
	gameData.nPlayers = nClients;
	ReleaseMutex(hMutexAddPlayer);

	return clientId;
}

int createGameDataPipe(int id) {
	HANDLE hGamePipe = INVALID_HANDLE_VALUE;
	BOOL isConnected = FALSE;
	TCHAR aux[STRINGBUFFERSIZE];
	LPCTSTR pipeName;

	_stprintf_s(aux, _countof(aux), PIPE_NAME_GAMEDATA, id);
	pipeName = aux;
	_tprintf(TEXT("%s\n"), pipeName);

	while (gameData.gameState != OFF) {
		hGamePipe = CreateNamedPipe(
			pipeName,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			NULL,
			NULL,
			0,
			NULL);

		if (hGamePipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("CreateNamedPipe failed. Error: %d\n"), GetLastError());
			return -1;
		}

		isConnected = ConnectNamedPipe(hGamePipe, NULL);
		if (!isConnected) {
			if (GetLastError() == ERROR_PIPE_CONNECTED)
				isConnected = TRUE;
			else
				isConnected = FALSE;
		}

		if (isConnected) {
			WaitForSingleObject(hMutexAddPlayer, INFINITE);
			clientsInfo[id].hGamePipe = hGamePipe;
			clientsInfo[id].isLocal = FALSE;
			ReleaseMutex(hMutexAddPlayer);
			_tprintf(TEXT("Last Error: %d\n"), GetLastError());
			_tprintf(TEXT("GamePipe successfully Created\n"));
			return 1;
		}
		else
			CloseHandle(hGamePipe);//if not connected no need for hPipe and goes to next iteration
	}
	return 0;
}

void movePlatform(int id, TCHAR direction) {
	WaitForSingleObject(hMutexGameDataShare, INFINITE);

	if (direction == TEXT('l')) {
		if (gameData.players[id].platform.x - PLATFORM_SPEED >= 0)
			gameData.players[id].platform.x = gameData.players[id].platform.x - PLATFORM_SPEED;
		else
			gameData.players[id].platform.x = 0;
	}
	else if (direction == TEXT('r')) {
		if (gameData.players[id].platform.x + PLATFORM_SIZE_X + PLATFORM_SPEED <= GAME_WIDTH)
			gameData.players[id].platform.x = gameData.players[id].platform.x + PLATFORM_SPEED;
		else
			gameData.players[id].platform.x = GAME_WIDTH - PLATFORM_SIZE_X;
	}

	ReleaseMutex(hMutexGameDataShare);
	updateGameData();
}

void ballTimer() {

	LARGE_INTEGER liDueTime;
	liDueTime.QuadPart = BALL_TIMER;

	if (!SetWaitableTimer(hBallTimer, &liDueTime, 0, NULL, NULL, 0)) {
		_tprintf(TEXT("SetWaitableTimer failed (%d)\n"), GetLastError());
	}

	if (WaitForSingleObject(hBallTimer, INFINITE) != WAIT_OBJECT_0)
		printf("WaitForSingleObject failed (%d)\n", GetLastError());
}

BOOL isTouchingPlatform(int id) {
	for (int i = 0; i < nClients; i++) {
		if (gameData.balls[id].y + BALL_SIZE > PLATFORM_START_Y && gameData.balls[id].y < PLATFORM_START_Y)
			if(gameData.balls[id].x + BALL_SIZE >= gameData.players[i].platform.x && gameData.balls[id].x <= gameData.players[i].platform.x + PLATFORM_SIZE_X)
				return true;
	}
	return FALSE;
}

BOOL isTouchingBrick(int id) {
	for (int i = 0; i < 30; i++) {
		if (gameData.balls[id].y == gameData.bricks[i].y + BRICK_SIZE_Y && gameData.balls[id].x + BALL_SIZE > gameData.bricks[i].x &&  gameData.balls[id].x < gameData.bricks[i].x + BRICK_SIZE_X && gameData.bricks[i].hp > 0) {
			gameData.bricks[i].hp = 0;
			return true;
		}
	}
	return false;
}

DWORD WINAPI BallThread(LPVOID param) {
	int id;
	WaitForSingleObject(hMutexGameDataShare, INFINITE);
	id = gameData.nBalls;
	gameData.nBalls++;
	ReleaseMutex(hMutexGameDataShare);

	int x, y, xSpeed, ySpeed, yCounter;
	xSpeed = BALL_SPEED;
	ySpeed = BALL_SPEED;
	x = 600;
	y = 600;
	yCounter = 0;

	while (gameData.gameState == GAME) {
		WaitForSingleObject(hMutexGameDataShare, INFINITE);
		gameData.balls[0].x = x;
		gameData.balls[0].y = y;
		ReleaseMutex(hMutexGameDataShare);

		updateGameData();
		ballTimer();

		x = x + xSpeed;
		y = y + ySpeed;

		

		if (x <= 0 || (x+BALL_SIZE) >= GAME_WIDTH)
			xSpeed *= -1;
		if (y <= 0 || (y+BALL_SIZE) >= GAME_HEIGHT || isTouchingPlatform(0) || isTouchingBrick(id))
			if (yCounter >= 10) {
				ySpeed *= -1;
				yCounter = 0;
			}

		yCounter++;
	}

	WaitForSingleObject(hMutexGameDataShare, INFINITE);
	gameData.nBalls--;
	ReleaseMutex(hMutexGameDataShare);
	_tprintf(TEXT("Bye Ball\n"));
	return 0;
}

DWORD WINAPI ReadMessages(LPVOID param) {
	Message aux;
	int clientId;

	while (gameData.gameState != OFF) {
		WaitForSingleObject(hSemaphoreCS, INFINITE);
		WaitForSingleObject(hMutexCtoS, INFINITE);
		aux = pBufferCtoS->message[pBufferCtoS->out];
		pBufferCtoS->out = (pBufferCtoS->out + 1) % MSGBUFFERSIZE;
		ReleaseMutex(hMutexCtoS);
		if (!ReleaseSemaphore(hSemaphoreCC, 1, NULL)) {
			_tprintf(TEXT("Release Semaphore Error: (%d). \n"), GetLastError());
		}

		switch (aux.header) {
			case 0:
				break;

			case 1 :	//LOGIN
				clientId = addUser(TRUE, aux.content.userName, NULL, NULL);
				if (clientId == -1) {
					aux.header = 2;
				}
				else {
					aux.id = clientId;
					aux.header = 1;
					sendMessage(aux);
					_tprintf(TEXT("%s ID --> %d.\n"), clientsInfo[clientId].name, clientId);
				}
				sendMessage(aux);
				break;

			case 3:
				movePlatform(aux.id, aux.content.direction);
				_tprintf(TEXT("%c ID --> %d.\n"), aux.content.direction, clientId);
				break;

			/*case 4:
				gameData.gameState = 4;
				updateGameData();
				break;*/
		}
	}
	_tprintf(TEXT("Bye Messages\n"));
	return 0;
}

DWORD WINAPI ReadPipedMessagesInstances(LPVOID param) {
	Message aux;
	int clientId;

	DWORD bytesRead = 0, replyBytes = 0, bytesWritten = 0;
	BOOL fSuccess = FALSE;
	HANDLE hMessagePipe = NULL;

	if (param == NULL) {
		_tprintf(TEXT("Error. No param value.\n"));
		return (DWORD)-1;
	}

	hMessagePipe = (HANDLE)param;

	fSuccess = FALSE;

	while (TRUE) {
		_tprintf(TEXT("A\n"));
		fSuccess = ReadFile(
			hMessagePipe,
			&aux,
			sizeof(Message),
			&bytesRead,
			NULL);
		_tprintf(TEXT("B\n"));
		if (!fSuccess || bytesRead == 0) {
			if (GetLastError() == ERROR_BROKEN_PIPE) 
				_tprintf(TEXT("Client is off. Error: %d\n"), GetLastError());
			else
				_tprintf(TEXT("ReadFile failed. Error: %d\n"), GetLastError());
			break;
		}

		switch (aux.header) {
			case 0:
			break;
			case 1:	//LOGIN
				_tprintf(TEXT("%s\n"), aux.content.userName);
				clientId = addUser(FALSE, aux.content.userName, hMessagePipe, NULL); //If it's denied
				if (clientId == -1) {
					_tprintf(TEXT("Clients are full.\n"));
					aux.id = clientId;
					aux.header = 2;
					aux.content.confirmation = FALSE;
					sendPipedMessageByHandle(hMessagePipe, aux);
				}
				else {                                                //if its accepted
					aux.id = clientId;
					aux.header = 2;
					aux.content.confirmation = TRUE;
					sendPipedMessageByHandle(hMessagePipe, aux);
					_tprintf(TEXT("%s ID --> %d. nClientes-->%d\n"), clientsInfo[clientId].name, clientId, nClients);
					createGameDataPipe(clientId);
				}
			break;
			case 2:
				_tprintf(TEXT("header = 2\n"));
				sendPipedMessageByHandle(hMessagePipe, aux);
				break;
			case 3:
				_tprintf(TEXT("%c ID --> %d.\n"), aux.content.direction, clientId);
				movePlatform(aux.id, aux.content.direction);
				_tprintf(TEXT("%c ID --> %d.\n"), aux.content.direction, clientId);
				break;
		}
	}

	FlushFileBuffers(hMessagePipe);
	DisconnectNamedPipe(hMessagePipe);
	CloseHandle(hMessagePipe);
	return 1;
}

DWORD WINAPI ReadPipedMessagesControl(LPVOID param) {
	HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
	BOOL isConnected = FALSE;
	DWORD dwThreadId;
	LPCTSTR pipeName = PIPE_NAME;

	while (gameData.gameState != OFF) {
		hPipe = CreateNamedPipe(
			pipeName,
			PIPE_ACCESS_DUPLEX,
			PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
			PIPE_UNLIMITED_INSTANCES,
			NULL,
			NULL,
			0,
			NULL);

		if (hPipe == INVALID_HANDLE_VALUE) {
			_tprintf(TEXT("CreateNamedPipe failed. Error: %d\n"), GetLastError());
			return -1;
			//maybe error? teorica do prof
		}

		isConnected = ConnectNamedPipe(hPipe, NULL);
		if (!isConnected) {
			if (GetLastError() == ERROR_PIPE_CONNECTED)
				isConnected = TRUE;
			else
				isConnected = FALSE;
		}

		if (isConnected) {
			hThread = CreateThread(
				NULL,
				0,
				ReadPipedMessagesInstances,
				(LPVOID)hPipe,
				0,
				NULL);

			if (hThread == NULL) {
				_tprintf(TEXT("Error creating thread: %d\n"), GetLastError());
				return -1;
			} else
				CloseHandle(hThread); //if it goes right there's no need for handle
		} else
			CloseHandle(hPipe);//if not connected no need for hPipe and goes to next iteration
	}
	return 0;
}

int _tmain(int argc, LPTSTR argv[]) {

#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
#endif

	TCHAR command[STRINGBUFFERSIZE];
	HANDLE hReadMessagesThread, hBallThread, hPipeReadThread;

	HKEY hKey;
	DWORD result;
	DWORD keyType = REG_BINARY;
	DWORD bufSize = sizeof(Top10);
	Top10 top10, testeResultado;
	

	if (RegCreateKeyEx(HKEY_CURRENT_USER, TEXT("Software\\ArkanoidTop10"), 0, NULL,
		REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hKey, &result) != ERROR_SUCCESS) {
		return -1;
	}


	initializeLocalMemory();
	initializeConfig();
	initializeClientInfo();
	setUpBricks1();

	gameData.gameState = LOGIN;
	gameData.nBalls = 0;
	nClients = 0;
	ballId = 0;
	
	hPipeReadThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipedMessagesControl, NULL, 0, NULL);
	hReadMessagesThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadMessages, NULL, 0, NULL);

	
	_tprintf(TEXT("Server Ready\n"));

	while (gameData.gameState != OFF) {
		fflush(stdin);
		_fgetts(command, STRINGBUFFERSIZE, stdin);
		command[_tcslen(command) - 1] = '\0';

		if (_tcscmp(command, TEXT("start")) == 0) {
			gameData.gameState = GAME;
			hBallThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BallThread, NULL, 0, NULL);
		}
		else if (_tcscmp(command, TEXT("top")) == 0) {
			/*gameData.score = 115;
			top10.score[0] = gameData.score;
			gameData.score = 245;
			top10.score[1] = gameData.score;*/
			RegSetValueEx(hKey, TEXT("Top10"), 0, REG_BINARY, (LPBYTE)&top10, sizeof(Top10));
			RegQueryValueEx(hKey, TEXT("Top10"), 0, &keyType, (LPBYTE)&testeResultado, &bufSize);

			_tprintf(TEXT("%d, %d"), testeResultado.score[0], testeResultado.score[1]);
		}
		else if (_tcscmp(command, TEXT("exit")) == 0) {
			CloseHandle(hMapFileStoC);
			gameData.gameState = OFF;
			ExitReadingThread();
			//WaitForSingleObject(hReadMessagesThread, INFINITE);
			//WaitForSingleObject(hBallThread, 1000);
		}
	}
}


