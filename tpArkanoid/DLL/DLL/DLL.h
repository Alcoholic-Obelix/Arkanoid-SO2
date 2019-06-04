#pragma once

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include "common.h"

//LocalClient
extern "C" DLL_API int LocalInitializeClientConnections();

extern "C" DLL_API int LocalLogin(TCHAR *name);

extern "C" DLL_API int LocalSendMessage(Message content);

extern "C" DLL_API int LocalReceiveMessage(Message* aux);

extern "C" DLL_API int LocalReceiveBroadcast(GameData* aux);

//External Client
extern "C" DLL_API int PipeInitialize();

extern "C" DLL_API int RemoteLogin(TCHAR *name);

extern "C" DLL_API int PipeReceiveMessage(Message *aux);

extern "C" DLL_API int PipeSendMessage(Message content);

extern "C" DLL_API int RemoteReceiveGameData(GameData *gameData);

