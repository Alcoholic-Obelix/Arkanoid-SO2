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
#include "resource.h"
#include "DLL.h"


GameData gameData;
BOOL isLocal;
int myId;
TCHAR myName[STRINGBUFFERSIZE];
TCHAR IP[15];

HANDLE hActualizaJogo;
HWND hWnd;
HINSTANCE hInstanceGlobal;
static HBITMAP hbit, ball, cover, limits;
static HBRUSH bLimits, bBackground, transparentBrush;
RECT rc;
int maxX, maxY, screenX, screenY;
HDC hdc, memdc, auxdc;
PAINTSTRUCT area;
TCHAR szProgName[] = TEXT("Base");


LRESULT CALLBACK TrataEventos(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

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
	HANDLE handle;

	while (gameData.gameState != OFF) {
		LocalReceiveBroadcast(&gameData);
		handle = (HANDLE)TrataEventos(hWnd, WM_PAINT, NULL, NULL);
		WaitForSingleObject(handle, INFINITE);
	}
	return 1;
}

DWORD WINAPI RemoteUpdateGameData(LPVOID param) {
	while (gameData.gameState != OFF) {
		RemoteReceiveGameData(&gameData);
	}
	return 1;

}

BOOL CALLBACK TrataLogin(HWND hDlg, UINT messg, WPARAM wParam, LPARAM lParam) {
	int in;
	switch (messg) {
		case WM_INITDIALOG:
			CheckRadioButton(hDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
			return TRUE;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case IDOK:
					if (IsDlgButtonChecked(hDlg, IDC_RADIO1) == BST_CHECKED) {
						isLocal = TRUE;
						GetDlgItemText(hDlg, IDC_EDIT1, myName, STRINGBUFFERSIZE);
						
						LocalInitializeClientConnections();
						for (int i = 0; i < LOGIN_TRIALS; i++) {
							in = LocalLogin(myName);
							if(in == 1)
								break;
							Sleep(1000);
						}

						if (in == 0)
							return FALSE;

						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadLocalMessages, NULL, 0, NULL);
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)LocalUpdateGameData, NULL, 0, NULL);
					}
					else {
						isLocal = FALSE;
						GetDlgItemText(hDlg, IDC_EDIT1, myName, STRINGBUFFERSIZE);
						GetDlgItemText(hDlg, IDC_IPADDRESS1, IP, 15);
						PipeInitialize();
						RemoteLogin(myName);
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)ReadPipedMessages, NULL, 0, NULL);
						CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)RemoteUpdateGameData, NULL, 0, NULL);
					}
					EndDialog(hDlg, 0);
					return TRUE;

				case IDCANCEL:
					EndDialog(hDlg, 0);
					return TRUE;

			}
			return TRUE;

		case WM_CLOSE:
			EndDialog(hDlg, 0);
			return TRUE;
	}


	return FALSE;
}


LRESULT CALLBACK TrataEventos(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	DWORD nBytes;
	HANDLE h;
	//Inicia Jogo
	int i, j, aux;
	TCHAR texto[100];

	switch (msg) {
		case WM_CREATE:
			bLimits = CreateSolidBrush(RGB(109, 121, 148));
			bBackground = CreateSolidBrush(RGB(230, 230, 230));
			ball = LoadBitmap(hInstanceGlobal, MAKEINTRESOURCE(IDB_BITMAP1));
			cover = LoadBitmap(hInstanceGlobal, MAKEINTRESOURCE(IDB_BITMAP2));
			maxX = GetSystemMetrics(SM_CXSCREEN);
			maxY = GetSystemMetrics(SM_CYSCREEN);
			hdc = GetDC(hWnd);
			memdc = CreateCompatibleDC(hdc);
			hbit = CreateCompatibleBitmap(hdc, maxX, maxY);
			SelectObject(memdc, hbit);
			ReleaseDC(hWnd, hdc);
			//hActualizaJogo = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)actualizaJogo, (LPVOID)hWnd, 0, NULL);
			break;

		case WM_DESTROY:
			PostQuitMessage(WM_QUIT);
			break;

		case WM_PAINT:
			InvalidateRect(hWnd, NULL, 1);
			hdc = BeginPaint(hWnd, &area);
			auxdc = CreateCompatibleDC(hdc);

			if (gameData.gameState == LOGIN) {
				_stprintf_s(texto, 100, TEXT("ARKANOID"));
				TextOut(hdc, 300, 100, texto, _tcslen(texto));
				SelectObject(auxdc, cover);
				BitBlt(hdc, 200, 0, 800, 554, auxdc, 0, 0, SRCCOPY);
			}
			else if (gameData.gameState == GAME) {
				SelectObject(auxdc, ball);
				BitBlt(hdc, gameData.balls[0].x, gameData.balls[0].y, BALL_SIZE, BALL_SIZE, auxdc, 0, 0, SRCCOPY);
			}

			/*SelectObject(auxdc, ball);
			BitBlt(hdc, x, y, 48, 48, auxdc, 0, 0, SRCCOPY);*/
			DeleteDC(auxdc);
			EndPaint(hWnd, &area);
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
				case ID_LOGIN://LOGIN					
					DialogBox(hInstanceGlobal, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)TrataLogin);
					break;

				case ID_EXIT: 
					PostQuitMessage(0);
					break;

				/*case IDD_DIALOG2:
					DialogBox(hInstanceGlobal, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)TrataDlg);
					break;*/
			}

			break;

		case WM_CLOSE: // Destruir a janela e terminar o programa
			// "PostQuitMessage(Exit Status)"
			PostQuitMessage(0);
			break;

		default:
			// Neste exemplo, para qualquer outra mensagem (p.e. "minimizar", "maximizar",
			// "restaurar") não é efectuado nenhum processamento, apenas se segue 
			// o "default" do Windows DefWindowProc()
			return(DefWindowProc(hWnd, msg, wParam, lParam));
			break;
	}
	return(0);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nCmdShow) {

	#ifdef UNICODE
	_setmode(_fileno(stdin), _O_WTEXT);
	_setmode(_fileno(stdout), _O_WTEXT);
	#endif



	HANDLE hReadMessagesThread, hUpdateGameDataThread;
	Message aux;
	gameData.gameState = LOGIN;

	MSG lpMsg; // MSG é uma estrutura definida no Windows para as mensagens
	WNDCLASSEX wcApp; // WNDCLASSEX é uma estrutura cujos membros servem para
	hInstanceGlobal = hInst;

	// definir as características da classe da janela
   // ============================================================================
   // 1. Definição das características da janela "wcApp"
   // (Valores dos elementos da estrutura "wcApp" do tipo WNDCLASSEX)
   // ============================================================================
	wcApp.cbSize = sizeof(WNDCLASSEX); // Tamanho da estrutura WNDCLASSEX
	wcApp.hInstance = hInst; // Instância da janela actualmente exibida
   // ("hInst" é parâmetro de WinMain e vem
   // inicializada daí)
	wcApp.lpszClassName = szProgName; // Nome da janela (neste caso = nome do programa)
	wcApp.lpfnWndProc = TrataEventos; // Endereço da função de processamento da janela
   // ("TrataEventos" foi declarada no início e
   // encontra-se mais abaixo)
	wcApp.style = CS_HREDRAW | CS_VREDRAW;// Estilo da janela: Fazer o redraw se for
   // modificada horizontal ou verticalmente
	wcApp.hIcon = LoadIcon(hInst, IDI_APPLICATION);// "hIcon" = handler do ícon normal
   //"NULL" = Icon definido no Windows
   // "IDI_AP..." Ícone "aplicação"
	wcApp.hIconSm = LoadIcon(hInst, IDI_WINLOGO);// "hIconSm" = handler do ícon pequeno
	//"NULL" = Icon definido no Windows
	// "IDI_INF..." Ícon de informação
	wcApp.hCursor = LoadCursor(hInst, IDC_ARROW); // "hCursor" = handler do cursor (rato)
   // "NULL" = Forma definida no Windows
   // "IDC_ARROW" Aspecto "seta"
	wcApp.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1); // Classe do menu que a janela pode ter
   // (NULL = não tem menu)
	wcApp.cbClsExtra = 0; // Livre, para uso particular
	wcApp.cbWndExtra = 0; // Livre, para uso particular
	wcApp.hbrBackground = (HBRUSH)GetStockObject(GRAY_BRUSH);
	// "hbrBackground" = handler para "brush" de pintura do fundo da janela. Devolvido por
	// "GetStockObject".Neste caso o fundo será branco
	// ============================================================================
	// 2. Registar a classe "wcApp" no Windows
	// ============================================================================
	if (!RegisterClassEx(&wcApp))
		return(0);
	// ============================================================================
	// 3. Criar a janela
	// ============================================================================

	hWnd = CreateWindow(
		szProgName, // Nome da janela (programa) definido acima
		TEXT("Arkanoid"),// Texto que figura na barra do título
		WS_OVERLAPPEDWINDOW, // Estilo da janela (WS_OVERLAPPED= normal)
		20, // Posição x pixels (default=à direita da última)
		10, // Posição y pixels (default=abaixo da última)
		WINDOW_WIDTH, // Largura da janela (em pixels)
		WINDOW_HEIGHT, // Altura da janela (em pixels)
		(HWND)HWND_DESKTOP, // handle da janela pai (se se criar uma a partir de
		// outra) ou HWND_DESKTOP se a janela for a primeira,
		// criada a partir do "desktop"
		(HMENU)NULL, // handle do menu da janela (se tiver menu)
		(HINSTANCE)hInst, // handle da instância do programa actual ("hInst" é
		// passado num dos parâmetros de WinMain()
		0); // Não há parâmetros adicionais para a janela
		// ============================================================================
		// 4. Mostrar a janela
		// ============================================================================

	ShowWindow(hWnd, nCmdShow); // "hWnd"= handler da janela, devolvido por
   // "CreateWindow"; "nCmdShow"= modo de exibição (p.e.
   // normal/modal); é passado como parâmetro de WinMain()
	UpdateWindow(hWnd); // Refrescar a janela (Windows envia à janela uma
   // mensagem para pintar, mostrar dados, (refrescar)…
   // ============================================================================
   // 5. Loop de Mensagens
   // ============================================================================
   // O Windows envia mensagens às janelas (programas). Estas mensagens ficam numa fila de
   // espera até que GetMessage(...) possa ler "a mensagem seguinte"
   // Parâmetros de "getMessage":
   // 1)"&lpMsg"=Endereço de uma estrutura do tipo MSG ("MSG lpMsg" ja foi declarada no
   // início de WinMain()):
   // HWND hwnd handler da janela a que se destina a mensagem
   // UINT message Identificador da mensagem
   // WPARAM wParam Parâmetro, p.e. código da tecla premida
   // LPARAM lParam Parâmetro, p.e. se ALT também estava premida
   // DWORD time Hora a que a mensagem foi enviada pelo Windows
   // POINT pt Localização do mouse (x, y)
   // 2)handle da window para a qual se pretendem receber mensagens (=NULL se se pretendem
   // receber as mensagens para todas as janelas pertencentes à thread actual)
   // 3)Código limite inferior das mensagens que se pretendem receber
   // 4)Código limite superior das mensagens que se pretendem receber
   // NOTA: GetMessage() devolve 0 quando for recebida a mensagem de fecho da janela,
   // terminando então o loop de recepção de mensagens, e o programa
	while (GetMessage(&lpMsg, NULL, 0, 0)) {
		TranslateMessage(&lpMsg); // Pré-processamento da mensagem (p.e. obter código
	   // ASCII da tecla premida)
		DispatchMessage(&lpMsg); // Enviar a mensagem traduzida de volta ao Windows, que
	   // aguarda até que a possa reenviar à função de
	   // tratamento da janela, CALLBACK TrataEventos (abaixo)
	}

	// ============================================================================
	// 6. Fim do programa
	// ============================================================================
	return((int)lpMsg.wParam); // Retorna sempre o parâmetro wParam da estrutura lpMsg
}







