#pragma once

#ifdef UNICODE
#define _tfgets fgetws
#else
#define _tfgets fgets
#de
#endif

#define CONFIGURATION_FILE TEXT("config.txt")
//Game State
#define OFF 0
#define LOGIN 1
#define GAME 2

//ClientState
#define EMPTY 0
#define LOGGED_IN 1

//OtherConfs
#define LOGIN_RECEIVE_TRIALS 5
#define LOGIN_TRIALS 10
#define LOGIN_WAIT_TIME 2000

//GameData
#define GAMEDATA_FILE_NAME TEXT("gd")
#define GAMEDATA_EVENT_FILE_NAME TEXT("evgd")
#define MUTEX_NAME_GAMEDATA_SHARE TEXT("mtgdshare")

//Server to Client
#define MAPPED_FILE_NAME_SC TEXT("FMSC")
#define SEMAPHORE_NAME_SC TEXT("ssc")
#define SEMAPHORE_NAME_CC TEXT("scc")
#define MUTEX_NAME_SC TEXT("mtsc")

//Client to Server
#define MAPPED_FILE_NAME_CS TEXT("FMCS")
#define SEMAPHORE_NAME_CS TEXT("scs")
#define SEMAPHORE_NAME_SS TEXT("sss")
#define MUTEX_NAME_CS TEXT("mtcs")

//NAMEDPIPES
#define PIPE_NAME TEXT("\\\\.\\pipe\\np")
#define PIPE_NAME_GAMEDATA TEXT("\\\\.\\pipe\\npgd%d")
#define MAX_CONECTION_TRIES 5
#define MUTEX_NAME_ADD_USER TEXT("mtau")


#define PIPEBUFFERSIZE 2048
#define STRINGBUFFERSIZE 64
#define MSGBUFFERSIZE 10
#define MAX_SEM_COUNT 10
#define MIN_SEM_COUNT 0

////////////////////////////////////
//////////////////GUI/////////
///////////////////////////////

//WINDOWS
#define WINDOW_WIDTH 800 //1475
#define WINDOW_HEIGHT 400 //775
#define GAME_WIDTH 1400
#define GAME_HEIGHT 750

//BALL
#define BALL_SIZE 20
#define BALL_SPEED 1
#define BALL_TIMER -100000LL

//PLATFORM
#define PLATFORM_SIZE_X 150
#define PLATFORM_SIZE_X 15
#define PLATFORM_SPEED 5





typedef struct config {
	int maxPlayers;
	int levels;
	int nLives;
	int ballSpeed;
	int nBricks;
	int powerUpSpeed;
	int nSpeedUps;
	int speedUpDuration;
	int nSlowDowns;
	int slowDownDuration;
	int nTriples;
	int tripleDuration;
} Config;

typedef struct top10 {
	TCHAR *name[10];
	int score[10];
} Top10;

typedef struct clientsInfo {
	BOOL isLocal;
	int state;
	TCHAR name[STRINGBUFFERSIZE];
	HANDLE hMessagePipe;
	HANDLE hGamePipe;
} ClientsInfo;

///////////////GAMEDATA
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

typedef struct player {
	int id;
	Platform platform;
	int score;
}Player;

typedef struct gameData {
	int gameState;
	int secondsToStart;
	Player players[20];
	Ball balls[3];
} GameData;

//MESSAGES
typedef struct move {
	TCHAR direction;   //l-left | 2-right   
};

typedef struct powerUp {
	char mode;
} PowerUp;

typedef union messageContent {			//0 Server to server for shutdown
	TCHAR userName[STRINGBUFFERSIZE];	//1 Client trying to login and server response
	bool confirmation;					//2
	PowerUp powerUp;					//3
	bool exit;							//4
} Content;

typedef struct message {
	int id;
	int header;
	Content content;
}Message;

typedef struct buffer {
	Message message[MSGBUFFERSIZE];
	int in;
	int out;
}Buffer;
