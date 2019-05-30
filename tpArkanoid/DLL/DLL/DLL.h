#pragma once

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include "common.h"

extern "C" DLL_API int InitializeClientConnections();

extern "C" DLL_API void Login();

extern "C" DLL_API int SendMessageToServer(Message content);

extern "C" DLL_API int ReceiveMessage(Message* aux);

extern "C" DLL_API int ReceiveBroadcast(GameData* aux);

