#pragma once

#ifdef DLL_EXPORTS
#define DLL_API __declspec(dllexport)
#else
#define DLL_API __declspec(dllimport)
#endif

#include "common.h"

extern "C" DLL_API int LocalInitializeClientConnections();

extern "C" DLL_API int LocalLogin(TCHAR *name);

extern "C" DLL_API int LocalSendMessageToServer(Message content);

extern "C" DLL_API int LocalReceiveMessage(Message* aux);

extern "C" DLL_API int LocalReceiveBroadcast(GameData* aux);

