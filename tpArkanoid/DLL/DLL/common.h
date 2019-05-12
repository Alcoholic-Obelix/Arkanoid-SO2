#pragma once

#define OFF 0
#define LOGIN 1
#define GAME 2

//GameData
#define GAMEDATA_FILE_NAME TEXT("gd")
#define GAMEDATA_EVENT_FILE_NAME TEXT("evgd")
//Server to Client
#define MAPPED_FILE_NAME_SC TEXT("FMSC")
#define SEMAPHORE_NAME_SC TEXT("ssc")
#define SEMAPHORE_NAME_CC TEXT("scc")
#define MUTEX_NAME_SC TEXT("mtsc")
#define MUTEX_NAME_CC TEXT("mtcc")

//Client to Server
#define MAPPED_FILE_NAME_CS TEXT("FMCS")
#define SEMAPHORE_NAME_CS TEXT("scs")
#define SEMAPHORE_NAME_SS TEXT("sss")
#define MUTEX_NAME_CS TEXT("mtcs")
#define MUTEX_NAME_SS TEXT("mtss")

#define STRINGBUFFERSIZE 64
#define MSGBUFFERSIZE 10
#define MAX_SEM_COUNT 10
#define MIN_SEM_COUNT 0

#define MAX_WIDTH 60
#define MAX_HEIGHT 25

typedef struct top10 {
	TCHAR *name[10];
	int score[10];
}Top10;

typedef struct ball {
	int x;
	int y;
	char mode;
} Ball;

typedef struct platform {
	int x;
	int y;
	char mode;
} Platform;

typedef struct gameData {
	int gameState;
	TCHAR userName[STRINGBUFFERSIZE];
	Ball ball;
	int score;
	Platform platform;
} GameData;

typedef struct powerUp {
	int x;
	int y;
	char mode;
} PowerUp;

typedef union messageContent {
	TCHAR userName[STRINGBUFFERSIZE];	//1
	bool confirmation;					//2
	PowerUp powerUp;					//3
	bool exit;							//4
} Content;

typedef struct message {
	int header;
	Content content;
}Message;


