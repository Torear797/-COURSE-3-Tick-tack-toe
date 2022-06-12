#define _CRT_SECURE_NO_WARNINGS
#include <Windows.h>
#include <WindowsX.h>
#include <CommCtrl.h>
#include <tchar.h>
#include <stdio.h>
#include <stdlib.h> 
#include <random>  
#include <process.h>
#include "psapi.h"
#include "strsafe.h"
#include <string>
#include "resource.h"
#include <iostream>
#include "gdiplus.h"
#include "gdipluspath.h"
#pragma comment (lib, "GdiPlus.lib")
using namespace Gdiplus;
ULONG_PTR token;
#pragma comment(lib, "Ws2_32.lib")
#define SizeCell 30
#pragma pack(1)
struct Cell
{
	unsigned short int X;
	unsigned short int Y;
	BOOL Value;
	BOOL block;
};
struct Pack
{
	unsigned short int X;
	unsigned short int Y;
	BOOL Win;
};
#pragma pack()
unsigned short int GlobalX = 0, GlobalY = 0, Draw = 0;
std::string IP = "127.0.0.1", PORT = "7777";
//std::string IP = "192.168.214.41", PORT = "7777"; //ТТТ1
//std::string IP = "192.168.214.47", PORT = "7777";// ТТТ2
Cell Cells[15][15];
BOOL FirstPlayer , CurrentMove, WinGame, Mclick, ExitGame , WinEnemy, IWin;
HWND  hMainForm , hGameField, Move, STATUS , hBackground;
//HBITMAP hBitmap;
Bitmap* WinBitmap;
HINSTANCE hinst;
BOOL TreadOk = TRUE;
HANDLE htread;
void Game_Init();
void Game_Shutdown();
void Game_Main(SOCKET s);
void MainFormCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify);
void ClearField(HDC hdc);
void DrawFlag(HDC hdc, unsigned short  int X, unsigned short  int Y);
void DrawOpponentsFlag(HDC hdc, unsigned short  int X, unsigned short  int Y);
void opponents(unsigned short int X, unsigned short  int Y);
BOOL IsWin();
void OnPaint(HDC hdc);
int sendn(SOCKET s, const char *buf, int len);
int recvn(SOCKET s, char *buf, int len);
INT_PTR WINAPI Menu(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
void opponents(unsigned short int X, unsigned short  int Y)
{
	Cells[X][Y].block = TRUE;
	HDC hdc = GetDC(hGameField);
	DrawOpponentsFlag(hdc, X, Y);
	ReleaseDC(hGameField, hdc);
}
INT_PTR WINAPI WinOrLoose(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		/*if(!WinEnemy && !IWin && Draw == 225) hBitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP6));
		else
		{
			if (FirstPlayer)
			{
				if (IWin)
					hBitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP4));
				else hBitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP5));
			}
			else
			{
				if (IWin)
					hBitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP5));
				else hBitmap = LoadBitmap(hinst, MAKEINTRESOURCE(IDB_BITMAP4));
			}
		} */
		if (!WinEnemy && !IWin && Draw == 225) WinBitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP6));
		else
		{
			if (FirstPlayer)
			{
				if (IWin)
					WinBitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP4));
				else WinBitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP5));
			}
			else
			{
				if (IWin)
					WinBitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP5));
				else WinBitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP4));
			}
		}
		hBackground = GetDlgItem(hWnd, IDC_WINBG);
	}
	break;
	case WM_PAINT:
	{
	/*	BITMAP bm;
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hBackground, &ps);
		HDC hdcMem = CreateCompatibleDC(hdc);
		HGDIOBJ hbmOld = SelectObject(hdcMem, hBitmap);
		GetObject(hBitmap, sizeof(bm), &bm);
		BitBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, SRCCOPY);
		SelectObject(hdcMem, hbmOld);
		DeleteDC(hdcMem);
		EndPaint(hBackground, &ps);*/

		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hBackground, &ps);
		if (NULL != WinBitmap)
		{
			Graphics g(hdc);
			g.DrawImage(WinBitmap, 0, 0);
		}
		EndPaint(hBackground, &ps);
	}
	break;
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
	}
	break;
	case WM_COMMAND: {
		HANDLE_WM_COMMAND(hWnd, wParam, lParam, MainFormCommand);
	}
	break;
	}
	return 0;
}
unsigned _stdcall Server(void *lparamter)
{
	Pack Msg;
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	sockaddr_in local_addr;
	local_addr.sin_family = AF_INET;
	local_addr.sin_port = htons(stoi(PORT));
	local_addr.sin_addr.s_addr = INADDR_ANY;

	bind(sock, (sockaddr *)&local_addr, sizeof(local_addr));
	SOCKET client_sock = NULL;
	int err = listen(sock, 1);

	sockaddr_in client_addr;
	int client_addr_size = sizeof(client_addr);
	
	if (SOCKET_ERROR != err)
	{	
		u_long argp = 1;
		err = ioctlsocket(sock, FIONBIO, &argp);
		if (SOCKET_ERROR != err) 
		{
			while ((client_sock = accept(sock, (sockaddr *)&client_addr, &client_addr_size)) && !ExitGame)
			{
				if (INVALID_SOCKET != client_sock)
				{
					CurrentMove = TRUE;
					SetWindowText(STATUS, TEXT("Ваш ход!"));
					SuspendThread(htread);
					break;
				}
			}
			argp = 0;
			ioctlsocket(sock, FIONBIO, &argp);
			ioctlsocket(client_sock, FIONBIO, &argp);
			while (!ExitGame)
			{
				Game_Main(client_sock);
			}
		}
	}
	if(client_sock != NULL)
	closesocket(client_sock);
	closesocket(sock);
	if (WinGame || Draw == 225) 
	PostMessage(hMainForm, WM_CLOSE, NULL, NULL);	
	return 0;
}
unsigned _stdcall Client(void *lparamter)
{
	SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	Pack Msg;
	sockaddr_in remote_addr;
	remote_addr.sin_family = AF_INET;
	remote_addr.sin_port = htons(stoi(PORT));
	remote_addr.sin_addr.s_addr = inet_addr(IP.c_str());
	TreadOk = FALSE;
	int s = -5;
	while (s != 0 && !ExitGame)
	s = connect(sock, (sockaddr *)&remote_addr, sizeof(remote_addr));
	if (s != SOCKET_ERROR) 
	{
		SetWindowText(STATUS, TEXT("Ход противника!"));
		while (!ExitGame)
		{
			Game_Main(sock);
		}
	} 
	TreadOk = TRUE;
	closesocket(sock);
	if (WinGame || Draw == 225)
	PostMessage(hMainForm, WM_CLOSE, NULL, NULL);
	return 0;
}
INT_PTR WINAPI Settings(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CTLCOLORSTATIC: 
	{
		HDC hdc = (HDC)wParam;
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(255, 255, 255));
		return (int)GetStockObject(NULL_BRUSH);
	}
	break;
	case WM_INITDIALOG:
	{
		SetDlgItemText(hWnd, IDC_IPADDRESS1, IP.c_str());
		SetDlgItemText(hWnd, IDC_PORT, PORT.c_str());
		hBackground = GetDlgItem(hWnd, IDC_SETTINGS);
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hBackground, &ps);
		Bitmap* bitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP3));
		if (NULL != bitmap)
		{
			Graphics g(hdc);
			g.DrawImage(bitmap, 0, 0);
		}
		delete bitmap;
		EndPaint(hBackground, &ps);
	}
	break;
	case WM_CLOSE:
	{
		EndDialog(hWnd, 0);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
	}
	break;
	case WM_COMMAND: {
		HANDLE_WM_COMMAND(hWnd, wParam, lParam, MainFormCommand);
	}
	break;
	}
	return 0;
}
INT_PTR WINAPI Menu(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		hBackground = GetDlgItem(hWnd, IDC_PICTURE);
	}
	break;
	//case WM_ERASEBKGND:
	//{
	//	//MessageBox(hMainForm, TEXT("ok"), TEXT("Сообщение"), MB_OK);
	//	//Image *image = ImageFromResource(hinst, MAKEINTRESOURCE(IDB_BITMAP3), RT_BITMAP);
	//	//Image*image = Image::FromFile(L"Menu.bmp");
	//	//Bitmap* bitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP3));
	//	//if (NULL != bitmap)
	//	{
	//		Graphics g((HDC)wParam);
	//		//g.DrawImage(bitmap, 0, 0);
	//		g.Clear(Color::White);
	//	}
	//	//delete bitmap;
	//}
	//break;
	case WM_PAINT:
	{	
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hBackground, &ps);
		Bitmap* bitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP3));
		if (NULL != bitmap)
		{
			Graphics g(hdc);
			g.DrawImage(bitmap, 0, 0);		
		}
		delete bitmap;
		EndPaint(hBackground, &ps);
	}
	break;
	case WM_CLOSE:
	{
		PostQuitMessage(WM_QUIT);
	}
	break;
	case WM_COMMAND: {
		HANDLE_WM_COMMAND(hWnd, wParam, lParam, MainFormCommand); 
	}
	break;
	}
	return 0;
}
INT_PTR WINAPI GameFieldPoc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	switch (Msg)
	{
	case WM_INITDIALOG:
	{
		hGameField = GetDlgItem(hWnd, IDC_Field);
		Move = GetDlgItem(hWnd, IDC_MOVE);
		STATUS = GetDlgItem(hWnd, IDC_STATUS);
		hMainForm = hWnd;
		Game_Init();	
	}
	break;
	case WM_LBUTTONDOWN:
	{	
		if (CurrentMove)
		{
			int widht = 0, Hight = 0;
			BOOL CellsOK = FALSE;
			for (int i = 0; i < 15; i++)
			{
				if (CellsOK) break;
				for (int j = 0; j < 15; j++)
				{
					if (LOWORD(lParam) >= Cells[i][j].X && LOWORD(lParam) <= Cells[i][j].X + (SizeCell - 5) && HIWORD(lParam) >= Cells[i][j].Y && HIWORD(lParam) <= Cells[i][j].Y + (SizeCell-5))
					{
						if (Cells[i][j].block == FALSE)
						{
							HDC hdc = GetDC(hGameField);
							DrawFlag(hdc, Cells[i][j].X, Cells[i][j].Y);
							ReleaseDC(hGameField, hdc);
							Cells[i][j].Value = 1;
							Cells[i][j].block = TRUE;
							GlobalX = i;
							GlobalY = j;
							Mclick = TRUE;
							CellsOK = TRUE;
							ResumeThread(htread);
							break;
						}
						else MessageBox(hGameField, TEXT("Клетка уже выбрана."), TEXT("Сообщение"), MB_OK);
					}
					widht += 30;
				}
				widht = 0;
				Hight += 30;
			}
		}
	}
	break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hGameField, &ps);
		Bitmap* bitmap = Bitmap::FromResource(hinst, MAKEINTRESOURCEW(IDB_BITMAP1));
		if (NULL != bitmap)
		{
			Graphics g(hdc);
			g.Clear(Color::White);
			g.DrawImage(bitmap, 0, 0);
		}
		delete bitmap;
		OnPaint(hdc);
		EndPaint(hGameField, &ps);
	}
	break;
	case WM_CLOSE:
	{	
		Game_Shutdown();
		EndDialog(hWnd, 0);
		if (WinGame) DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_WINORLOOSE), NULL, WinOrLoose, NULL);
		else DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
	}
	break;
	case WM_COMMAND: {
		HANDLE_WM_COMMAND(hWnd, wParam, lParam, MainFormCommand);
	}
	break;
	}
	return 0;
}
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE NotUsed, LPSTR pCmdLine, int fShow)
{
	hinst = hInstance;
	GdiplusStartupInput gdiplusStartupInput;
	GdiplusStartupOutput gdiplusStartupOutput;

	Status stret = GdiplusStartup(&token, &gdiplusStartupInput, NULL);

	if (stret == Ok)
	{
		WSADATA wsaData;
		WSAStartup(MAKEWORD(2, 2), &wsaData);
		DialogBoxParamW(hInstance, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
		delete WinBitmap;
		GdiplusShutdown(token);
		WSACleanup();
	}
	return EXIT_SUCCESS;
}
void MainFormCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
	switch (id)
	{
	case IDC_BUTTON8: //Главное меню из игры
	{
		ExitGame = TRUE;
		PostMessage(hMainForm, WM_CLOSE, NULL, NULL);
	}
	break;
	case IDC_BUTTON1: //Главное меню
	{
		EndDialog(hwnd, 0);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
	}
	break;
	case IDC_BUTTON6: //Переход в Настройки
	{
		EndDialog(hwnd, 0);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_DIALOG1), NULL, Settings, NULL);
	}
	break;
	case IDC_CANCELSETTINGS: { //Отменить настройки
		EndDialog(hwnd, 0);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
	}
	break;
	case IDC_GETIP: //Получить IP
	{
		TCHAR HostName[1024];
		GetDlgItemText(hwnd, IDC_EDIT3, HostName, 1024);
		hostent *remoteHost = gethostbyname(HostName);
		TCHAR szName[MAX_PATH];
		if (NULL == remoteHost || AF_INET != remoteHost->h_addrtype) MessageBox(hwnd, TEXT("Не корректное имя!"), TEXT("Сообщение"), MB_OK);
		else 
		{
			StringCchCopyA(szName, MAX_PATH, inet_ntoa(*(struct in_addr*)remoteHost->h_addr_list[0]));
			SetDlgItemText(hwnd, IDC_IPADDRESS1, szName);
		}
	}
	break;
	case IDC_BUTTON7: //Применить настройки
	{
		EndDialog(hwnd, 0);
		PORT.resize(::GetWindowTextLengthA(::GetDlgItem(hwnd, IDC_PORT)));::GetWindowTextA(::GetDlgItem(hwnd, IDC_PORT), &*PORT.begin(), ::GetWindowTextLengthA(::GetDlgItem(hwnd, IDC_PORT)) + 1 );
		IP.resize(::GetWindowTextLengthA(::GetDlgItem(hwnd, IDC_IPADDRESS1))); ::GetWindowTextA(::GetDlgItem(hwnd, IDC_IPADDRESS1), &*IP.begin(), ::GetWindowTextLengthA(::GetDlgItem(hwnd, IDC_IPADDRESS1)) + 1);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(IDD_MAINFORM), NULL, Menu, NULL);
	}
	break;
	case IDC_BUTTON5: //Выход
	{
		PostQuitMessage(0);
	}
	break;
	case IDC_BUTTON4: //Выход во время игры
	{
		Game_Shutdown();
		PostQuitMessage(0);
	}
	break;
	case IDC_Newgame: //Начать сначала
	{
		EndDialog(hwnd, 0);
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(GAMEFIELD), NULL, GameFieldPoc, NULL);
	}
	break;
	case IDC_BUTTON2: //Создать игру 
	{	
		if (TreadOk) 
		{
			EndDialog(hwnd, 0);
			FirstPlayer = TRUE;
			DialogBoxParamW(hinst, MAKEINTRESOURCEW(GAMEFIELD), NULL, GameFieldPoc, NULL);
		} else MessageBox(hMainForm, TEXT("Заверашем предыдущее соединение. Пожалуйста подождите."), TEXT("Сообщение"), MB_OK);
	}
	break;
	case IDC_BUTTON3: //Присоединится к игре
	{
		EndDialog(hwnd, 0);	
		FirstPlayer = FALSE;
		DialogBoxParamW(hinst, MAKEINTRESOURCEW(GAMEFIELD), NULL, GameFieldPoc, NULL);	
	}
	break;
	}
}
void OnPaint(HDC hdc)
{
	Graphics g(hdc);
	g.SetSmoothingMode(SmoothingModeHighQuality);
	Rect rect;
	g.GetVisibleClipBounds(&rect);
	Bitmap backBuffer(rect.Width, rect.Height, &g);
	Graphics temp(&backBuffer);
	Pen Contur(Color(255, 0, 0, 0), 1.f);
	SolidBrush Material(Color(150, 255, 255, 255));
	unsigned short int widht = 0, Hight = 0;
	for (unsigned short int i = 0; i < 15; ++i)
	{
		for (unsigned short int j = 0; j < 15; ++j)
		{
			temp.FillRectangle(&Material, 10 + widht, 10 + Hight, 25, 25);
			temp.DrawRectangle(&Contur, 10 + widht, 10 + Hight, 25, 25);
			widht += SizeCell;
		}
		widht = 0;
		Hight += SizeCell;
	}
	g.DrawImage(&backBuffer, rect);
}
void ClearField(HDC hdc)
{
	Graphics g(hdc);
	g.Clear(Color::Transparent);
}
void DrawFlag(HDC hdc, unsigned short int X, unsigned short int Y)
{
	Graphics g(hdc);
	g.SetSmoothingMode(SmoothingModeHighQuality);

	Pen Rect(Color::Green, 3.f);
	Pen Elips(Color::Red, 3.f);

	if (FirstPlayer) 
	{
		g.DrawLine(&Rect, X, Y, X + SizeCell - 5, Y + SizeCell - 5);
		g.DrawLine(&Rect, X ,Y + SizeCell - 5,  X + SizeCell - 5, Y);
	}
	else 
	{
		g.DrawEllipse(&Elips, X, Y, 25, 25);
	}
}
void DrawOpponentsFlag(HDC hdc, unsigned short int i, unsigned short int j)
{
	Graphics g(hdc);
	g.SetSmoothingMode(SmoothingModeHighQuality);

	Pen Rect(Color::Green,3.f);
	Pen Elips(Color::Red, 3.f);

	if (FirstPlayer)
	{
		g.DrawEllipse(&Elips, Cells[i][j].X, Cells[i][j].Y, 25, 25);
	}
	else
	{
		g.DrawLine(&Rect, Cells[i][j].X, Cells[i][j].Y, Cells[i][j].X + SizeCell - 5, Cells[i][j].Y + SizeCell - 5);
		g.DrawLine(&Rect, Cells[i][j].X, Cells[i][j].Y + SizeCell - 5, Cells[i][j].X + SizeCell - 5, Cells[i][j].Y);
	}
}
BOOL IsWin()
{
	unsigned short int Score = 0;
	for (unsigned short int i = 0; i < 15; ++i)//Горизонт
	{
		for (unsigned short int j = 0; j < 15; ++j)
		{
			if (Cells[i][j].Value == 1) 
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
		}
		Score = 0;
	}

	Score = 0;
	for (unsigned short int i = 0; i < 15; ++i)//Вертикаль
	{
		for (unsigned short int j = 0; j < 15; ++j)
		{
			if (Cells[j][i].Value == 1) 
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
		}
		
		Score = 0;
	}

	unsigned short int i2 = 0; Score = 0;
	for (unsigned short int i = 0; i < 11; ++i)//Слева на право(\\\\) - верхняя часть.
	{
		for (unsigned short int j = i; j < 15; ++j)
		{
			if (Cells[i2][j].Value == 1)
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
			i2++;
		}
		i2 = 0;
		Score = 0;
	}

	Score = 0;
	for (unsigned short int i = 1; i < 11; ++i)//Слева на право(\\\\\) - Нижняя часть.
	{
		i2 = i;
		for (unsigned short int j = 0; j < 15; ++j)
		{
			if (Cells[i2][j].Value == 1)
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
			i2++;
		}
		Score = 0;
	}

	Score = 0; i2 = 0;
	for (short int i = 15; i >= 0; --i)//(/////) - Верхняя часть.
	{
		for (short int j = i; j >= 0; --j)
		{
			if (Cells[i2][j].Value == 1)
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
			i2++;
		}
		i2 = 0;
		Score = 0;
	}

	Score = 0; 
	for (unsigned short int i = 0; i < 11; ++i)//(/////) - Нижняя часть.
	{
		i2 = i;
		for (short int j = 15; j >= 0; --j)
		{
			if (Cells[j][i2].Value == 1)
			{
				Score++;
				if (Score == 5) return TRUE;
			}
			else Score = 0;
			i2++;
		}
		i2 = 0;
		Score = 0;
	}

	Draw = 0;
	for (unsigned short int i = 0; i < 15; ++i)//Ничья
	{
		for (unsigned short int j = 0; j < 15; ++j)
		{
			if (Cells[i][j].block)Draw++;	
		}
	}
	return FALSE;
}
int sendn(SOCKET s, const char *buf, int len)
{
	int size = len;
	while (size > 0)
	{
		int n = send(s, buf, size, 0);
		if (n <= 0)
		{
			if (WSAGetLastError() == WSAEINTR)
			{
				continue;
			}
			return SOCKET_ERROR;
		}
		buf += n;
		size -= n;
	}
	return len;
}
int recvn(SOCKET s, char *buf, int len)
{
	int size = len;
	while (size > 0)
	{
		int n = recv(s, buf, size, 0);
		if (n <= 0)
		{
			if (WSAGetLastError() == WSAEINTR)
			{
				continue;
			}
			return SOCKET_ERROR;
		}
		buf += n;
		size -= n;
	}
	return len;
}
void Game_Init() 
{
	WinGame = FALSE;
	CurrentMove = FALSE;
	Mclick = FALSE;
	ExitGame = FALSE;
	WinEnemy = FALSE;
	IWin = FALSE;
	WSASetLastError(0);
	Draw = 0;
	unsigned short int widht = 0, Hight = 0;
	for (unsigned short int i = 0; i < 15; ++i)
	{
		for (unsigned short int j = 0; j < 15; ++j)
		{
			Cells[i][j].X = 10 + widht;
			Cells[i][j].Y = 10 + Hight;
			Cells[i][j].Value = 0;
			Cells[i][j].block = FALSE;
			widht += SizeCell;
		}
		widht = 0;
		Hight += SizeCell;
	}
	SetWindowText(STATUS, TEXT("Ожидание подключения!"));
	if (FirstPlayer) 
	{
		SetWindowText(Move, TEXT("X"));
		htread = (HANDLE)_beginthreadex(NULL, 0, Server, NULL, 0, NULL);
	}
	else 
	{
		SetWindowText(Move, TEXT("0"));
		htread = (HANDLE)_beginthreadex(NULL, 0, Client, NULL, 0, NULL);
	}
}
void Game_Shutdown() 
{
	ExitGame = TRUE;
	ResumeThread(htread);
	if (htread != NULL)
	{
		CloseHandle(htread);
		htread = NULL;
	}
}
void Game_Main(SOCKET s)
{
	Pack Msg;
	if (CurrentMove == FALSE)
	{
		int err = recvn(s, (char *)&Msg, sizeof(Msg));
		if (err == SOCKET_ERROR)
		{
			int error = WSAGetLastError();
			if (error = WSAECONNRESET)
			{
				if (!ExitGame)
				MessageBox(hMainForm, TEXT("Ваш противник сдался, вы будите перенаправлены в главное меню."), TEXT("Сообщение"), MB_OK);
				ExitGame = TRUE;
				PostMessage(hMainForm, WM_CLOSE, NULL, NULL);
			}
		}
		else
			{
				opponents(Msg.X, Msg.Y);
				Mclick = FALSE;
				IWin = IsWin();
				WinEnemy = Msg.Win;
				if (Msg.Win || Draw == 225)
				{
					CurrentMove = FALSE;
					WinGame = TRUE;
					ExitGame = TRUE;
				}
				CurrentMove = TRUE;
				SetWindowText(STATUS, TEXT("Ваш ход!"));
				if(!WinGame)
				SuspendThread(htread);
			}
	}
	else
	{
		if (Mclick)
		{
			Msg.X = GlobalX;
			Msg.Y = GlobalY;
			Msg.Win = IsWin();
			sendn(s, (char *)&Msg, sizeof(Msg));
			IWin = Msg.Win;
			if (Msg.Win || Draw == 225)
			{
				WinGame = TRUE;
				ExitGame = TRUE;
			}
			Mclick = FALSE;
			CurrentMove = FALSE;
			SetWindowText(STATUS, TEXT("Ход противника!"));
		}
	}
}