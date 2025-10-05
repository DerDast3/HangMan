#include "pch.h"
#include "worte.h"
#include "hangman.h"

WORD PRGNAME[] = L"HANGMAN";
WORD VERSION[] = L"2.10 dast 05.10.2025";

#define IDM_TIMER   WM_USER+8000
#define MAXGALG 11

LRESULT WINAPI WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

HWND mainhwnd;
HINSTANCE hInst;
UINT_PTR mytimer;
int Galgen = 4;
int Sieg = 0;
int Versuch = 0;
int Fehler = 0;
WORD CurrentWort[512];
WORD ShowWort[512];
WORD SourceWort[512];
WORD Missing[512];
BYTE Small[] = "abcdefghijklmnopqrstuvwxyzäöüß";
BYTE Large[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜß";
WORD SmallW[] = L"abcdefghijklmnopqrstuvwxyzäöüß";
WORD LargeW[] = L"ABCDEFGHIJKLMNOPQRSTUVWXYZÄÖÜß";
int PWidth;
int PHeight;
RECT letter[128];
POINT ochar[128];
RECT repaint;
int xfont;
int yfont;
int fontkind;
int OverLetter = 0;
int LastOverLetter = 0;
int DownLetter = 0;
int UpLetter = 0;
int LetterPos = 0;

#define TRACEWND_CLASSNAME L"MfxTraceWindow"
#define ID_COPYDATA_TRACEMSG MAKELONG(MAKEWORD('d','a'),MAKEWORD('s','t'))

int prints(LPCWSTR fmt, ...)
{
	HWND TraceWin;
	WORD writestr[4096];
	if ((TraceWin = FindWindow((wchar_t*)TRACEWND_CLASSNAME, NULL)) != NULL)
	{
		COPYDATASTRUCT cds;
		cds.dwData = ID_COPYDATA_TRACEMSG;
		cds.cbData = 1 + wvsprintf(writestr, fmt, (LPSTR)((&fmt) + 1));
		cds.lpData = (LPVOID)writestr;
		SendMessage((HWND)TraceWin, WM_COPYDATA, (WPARAM)NULL, (LPARAM)&cds);
	}
	return 1;
}


LPSTR MyErrString(int le)
{
	static BYTE ErrMess[2048];
	LPSTR mptr = ErrMess;
	LPVOID em;
	DWORD ret = 0;

	if (le == -1) le = GetLastError();
	ret = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL, le,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&em, 0, NULL);
	if (!ret)
	{
		sprintf_s(ErrMess, countW(ErrMess), "Error %u", le);
	}
	else
	{
		if (ret > 510) ret = 510;
		if (ret > 60) { *mptr++ = 13; *mptr++ = 10; }
		memcpy(mptr, (LPSTR)em, ret);
		mptr[ret] = 0;
		LocalFree(em);
	}

	return ErrMess;
}


void RegisterMyClass()
{
	WNDCLASS wc;

	memset(&wc, 0, sizeof(wc));
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInst;
	wc.hIcon = LoadIcon(hInst, (wchar_t*)L"HANGMAN");
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = (wchar_t*)L"HANGMANCLASS";
	RegisterClass(&wc);
}


int XFak(int x) { return ((x * PWidth) / 800); }
int YFak(int y) { return ((y * PHeight) / 600); }


void CenterWindowOnScreen(HWND hwnd, int xp, int yp)
{
	RECT rect;
	int xx, yy;
	GetWindowRect(hwnd, &rect);
	xx = GetSystemMetrics(SM_CXSCREEN);
	yy = GetSystemMetrics(SM_CYSCREEN);
	xx = (xx - (rect.right - rect.left)) >> 1;
	yy = (yy - (rect.bottom - rect.top)) >> 1;
	SetWindowPos(hwnd, NULL, xx + xp, yy + yp, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}


void InitResource()
{
	int xx, yy;
	RECT rect;
	RegisterMyClass();
	xx = GetSystemMetrics(SM_CXSCREEN);
	yy = GetSystemMetrics(SM_CYSCREEN);
	SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
	PWidth = rect.right - rect.left;
	PHeight = rect.bottom - rect.top;
	if (PWidth > 800) { PWidth = 800; xx = (xx - PWidth) >> 1; }
	else xx = rect.left;
	if (PHeight > 600) { PHeight = 600; yy = (yy - PHeight) >> 1; }
	else yy = rect.top;
	mainhwnd = CreateWindow((wchar_t*)L"HANGMANCLASS", (wchar_t*)L"Hangman",
		// WS_BORDER |
		WS_THICKFRAME |
		WS_OVERLAPPED | WS_CAPTION |
		WS_SYSMENU |
		WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
		xx, yy, PWidth, PHeight,
		NULL, NULL, hInst, NULL);
}


void ExitResource()
{
	if ((mainhwnd) && (IsWindow(mainhwnd))) DestroyWindow(mainhwnd);
	UnregisterClass((LPCWSTR)PRGNAME, hInst);
}


int __stdcall wWinMain(HINSTANCE hinstCurrent, HINSTANCE hinstPrevious, LPWSTR lpCmdLine, int nCmdShow)
{
	MSG msg;
	hInst = hinstCurrent;

	InitResource();
	memset(letter, 0, sizeof(letter));

	if (!mainhwnd) return 0;

	ShowWindow(mainhwnd, /* SW_SHOW */ nCmdShow);
	UpdateWindow(mainhwnd);

	while ((mainhwnd) && (IsWindow(mainhwnd)) &&
		(GetMessage(&msg, (HWND)NULL, 0, 0)))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	ExitResource();
	return 0;
}


BYTE MakeUpper(BYTE b)
{
	LPBYTE ss = Small;
	LPBYTE ll = Large;
	while ((*ss) && (*ll))
	{
		if (*ss == b) return *ll;
		ss++; ll++;
	}
	return b;
}


WORD MakeUpperW(WORD b)
{
	LPWORD ss = SmallW;
	LPWORD ll = LargeW;
	while ((*ss) && (*ll))
	{
		if (*ss == b) return *ll;
		ss++; ll++;
	}
	return b;
}


void MakeUpperStr(LPSTR what)
{
	while (*what)
	{
		*what = MakeUpper(*what);
		what++;
	}
}


void MakeUpperStrW(LPWORD what)
{
	while (*what)
	{
		*what = MakeUpperW(*what);
		what++;
	}
}


void InitGame(HWND hwnd)
{
	size_t i;
	size_t len;
	lstrcpy(CurrentWort, GimmeOne());
	lstrcpy(Missing, GimmeOne());
	MakeUpperStrW(CurrentWort);
	len = wcslen(CurrentWort);
	memset(ShowWort, 0, sizeof(ShowWort));
	for (i = 0; i < len; i++) ShowWort[i] = '_';
	Galgen = 0; Sieg = 0; Versuch = 0; Fehler = 0;
	lstrcpy(SourceWort, LargeW);
	PlaySound((wchar_t*)L"INIT", hInst,
		SND_RESOURCE | SND_NOWAIT | SND_NODEFAULT | SND_ASYNC);
	InvalidateRect(hwnd, NULL, TRUE);
	// prints("CurrentWort '%s'\r\n", CurrentWort);
}


void GalgenAt(HWND hwnd, HDC hdc)
{
	HDC hdcmem, hdcs;
	HBITMAP hbmp;
	HBITMAP hbmpold;
	WORD hlp[128];

	if ((Galgen < 0) || (Galgen > MAXGALG) || (Sieg)) Galgen = 0;
	if (Sieg) lstrcpy(hlp, (wchar_t*)L"pokal");
	else swprintf_s(hlp, countW(hlp), (wchar_t*)L"GALG%02u", Galgen);

	hdcs = CreateDC((wchar_t*)L"DISPLAY", NULL, NULL, NULL);
	hdcmem = CreateCompatibleDC(hdcs);
	hbmp = LoadBitmap(hInst, hlp);
	hbmpold = SelectObject(hdcmem, hbmp);

	if (Sieg)
		StretchBlt(hdc, XFak(210), YFak(151), XFak(380), YFak(298),
			hdcmem, 0, 0, 380, 298, SRCCOPY);
	else
		StretchBlt(hdc, XFak(254), YFak(130), XFak(292), YFak(340),
			hdcmem, 0, 0, 292, 340, SRCCOPY);

	SelectObject(hdcmem, hbmpold);
	DeleteObject(hbmp);
	DeleteDC(hdcmem);
	DeleteDC(hdcs);
}


void PressChar(HWND hwnd, WPARAM wParam)
{
	size_t  i, j, k;
	size_t len;
	BYTE b;
	if ((wParam < 33) || (wParam > 255)) return;
	b = MakeUpper((BYTE)wParam);
	len = wcslen(SourceWort);
	for (i = 0, j = -1; i < len; i++)
	{
		if (SourceWort[i] == b) { j = i; break; }
	}
	if (j < 0) return;
	Versuch++;
	SourceWort[j] = ' ';
	len = wcslen(ShowWort); Missing[len] = 0;
	for (i = 0, j = 0, k = 0; i < len; i++)
	{
		if (CurrentWort[i] == b) { j = 1; ShowWort[i] = b; }
		if (ShowWort[i] == '_')
		{
			k++;
			Missing[i] = CurrentWort[i];
		}
		else
		{
			Missing[i] = ' ';
		}
	}
	if ((!j) && (Galgen < MAXGALG))
	{
		Galgen++; Fehler++;
		PlaySound(Galgen >= MAXGALG ? (wchar_t*)L"LOST" : (wchar_t*)L"OHOHH", hInst,
			SND_RESOURCE | SND_NOWAIT | SND_NODEFAULT | SND_ASYNC);
	}
	else
		if (!k)
		{
			Sieg = 1;
			PlaySound((wchar_t*)L"APPLAUS", hInst,
				SND_RESOURCE | SND_NOWAIT | SND_NODEFAULT | SND_ASYNC);

		}
		else
		{
			PlaySound((wchar_t*)L"PRESS", hInst,
				SND_RESOURCE | SND_NOWAIT | SND_NODEFAULT | SND_ASYNC);
		}
	InvalidateRect(hwnd, NULL, TRUE);
}


int GetLetter(HWND hwnd, int x, int y)
{
	LPWORD cc;
	int i;
	POINT pt;
	pt.x = x; pt.y = y;
	for (i = 0, cc = SourceWort; *cc; i++, cc++)
	{
		if ((*cc > ' ') && (PtInRect(letter + i, pt)))
		{
			LetterPos = i;
			return *cc;
		}
	}
	return 0;
}


HFONT MyCreateFont(int ysize, int xsize, int font)
{
	return CreateFont(ysize, xsize,
		0, 0, FW_BOLD, 0, 0, 0, ANSI_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		font == 1 ? FIXED_PITCH | FF_MODERN :
		font == 2 ? VARIABLE_PITCH | FF_ROMAN :
		font == 3 ? VARIABLE_PITCH | FF_SWISS :
		DEFAULT_PITCH | FF_DONTCARE,
		font == 1 ? L"Courier New" :
		font == 2 ? L"Times New Roman" :
		font == 3 ? L"Arial" : L"Arial");
}


void SpruchAt(HDC hdc, int x, int y, int stepx, int stepy, wchar_t* str, int size, int font, int sample)
{
	int bkmode;
	HFONT hfo, oldhfo;
	int oldal;
	size_t len = wcslen(str);
	int xsize, ysize;
	COLORREF oldcol;

	ysize = YFak(size);
	xsize = XFak(font == 1 ? (134 * size) / 256 :
		font == 2 ? (97 * size) / 256 :
		font == 3 ? (107 * size) / 256 : (107 * size) / 256);

	if (sample)
	{
		xfont = xsize;
		yfont = ysize;
		fontkind = font;
	}

	oldcol = SetTextColor(hdc, RGB(0, 0, 0));
	bkmode = SetBkMode(hdc, TRANSPARENT);
	hfo = MyCreateFont(ysize, xsize, font);
	oldhfo = SelectObject(hdc, hfo);

	if ((x < 0) && (stepx > 0) && (len))
	{
		x = (INT)((PWidth - ((len - 1) * stepx)) >> 1);
	}

	oldal = SetTextAlign(hdc, TA_CENTER);
	if (!stepx)
	{
		TextOut(hdc, x < 0 ? (PWidth / 2) : x, y, str, (int)wcslen(str));
	}
	else
	{
		int idx = 0;
		while (*str)
		{
			if (sample)
			{
				letter[idx].left = x - xsize / 2 - 2;
				letter[idx].right = x + xsize / 2 + 2;
				letter[idx].top = y;
				letter[idx].bottom = y + ysize;
				ochar[idx].x = x;
				ochar[idx].y = y;
			}
			TextOut(hdc, x, y, str, 1); x += stepx; y += stepy;
			str++; idx++;
		}
	}
	SetTextAlign(hdc, oldal);

	SelectObject(hdc, oldhfo);
	DeleteObject(hfo);
	SetBkMode(hdc, bkmode);
	SetTextColor(hdc, oldcol);
}


void HighLight(HWND hwnd, int OverLetter)
{
	HDC hdc;
	HFONT hfo, oldhfo;
	WORD str[2];
	int x, y;
	int bkmode;
	int oldal;
	COLORREF oldcol;

	hdc = GetDC(hwnd);
	str[0] = (BYTE)OverLetter; str[1] = 0;
	bkmode = SetBkMode(hdc, TRANSPARENT);
	hfo = MyCreateFont(yfont, xfont, fontkind);
	oldhfo = SelectObject(hdc, hfo);
	x = ochar[LetterPos].x;
	y = ochar[LetterPos].y;
	oldal = SetTextAlign(hdc, TA_CENTER);
	repaint = letter[LetterPos];

	oldcol = SetTextColor(hdc, RGB(255, 128, 0));
	TextOut(hdc, x - 1, y - 1, str, 1);
	TextOut(hdc, x + 1, y - 1, str, 1);
	TextOut(hdc, x - 1, y + 1, str, 1);
	TextOut(hdc, x + 1, y + 1, str, 1);
	SetTextColor(hdc, oldcol);
	TextOut(hdc, x, y, str, 1);

	SetTextAlign(hdc, oldal);
	SelectObject(hdc, oldhfo);
	DeleteObject(hfo);
	SetBkMode(hdc, bkmode);
	ReleaseDC(hwnd, hdc);
}


void Paintit(HWND hwnd, HDC hdc)
{
	WORD hlp[128];
	GalgenAt(hwnd, hdc);
	if (Galgen >= MAXGALG)
		SpruchAt(hdc, -1, YFak(15), XFak(30), 0, Missing, 18, 1, 0);
	SpruchAt(hdc, -1, YFak(20), XFak(30), 0, ShowWort, 28, 1, 0);
	if (Versuch)
	{
		if ((!Sieg) && (Galgen < MAXGALG))
			swprintf_s(hlp, countW(hlp), L"%u. Versuch,  %u Fehler", Versuch, Fehler);
		else
			swprintf_s(hlp, countW(hlp), (Versuch == 1)
				? L"%u Versuch,  %u Fehler"
				: L"%u Versuche,  %u Fehler",
				Versuch, Fehler);
		SpruchAt(hdc, -1, YFak(70), 0, 0, hlp, 16, 3, 0);
	}
	SpruchAt(hdc, -1, YFak(500), XFak(24), 0, SourceWort, 28, 1, 1);
	SpruchAt(hdc, -1, YFak(545), 0, 0, L"Einen der Buchstaben auf der Tastatur drücken.     F1 für ein neues Spiel.", 20, 3, 0);
}


LRESULT WINAPI WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	MINMAXINFO* pmima;

	switch (message)
	{
	case WM_TIMER:
		if (wParam == IDM_TIMER)
		{
			if (mytimer) KillTimer(hwnd, mytimer); mytimer = 0;
			if (!IsRectEmpty(&repaint))
			{
				InvalidateRect(hwnd, &repaint, TRUE);
				SetRect(&repaint, 0, 0, 0, 0);
				LastOverLetter = 0;
			}
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		Paintit(hwnd, hdc);
		EndPaint(hwnd, &ps);
		break;

	case WM_CREATE:
		InitGame(hwnd);
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	case WM_ENDSESSION:
		DestroyWindow(hwnd);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_F1)
		{
			InitGame(hwnd);
		}
		return 0;

	case WM_CHAR:
		if ((Galgen < MAXGALG) && (!Sieg))
		{
			PressChar(hwnd, wParam);
		}
		return 0;

	case WM_SIZE:
		GetWindowRect(hwnd, &rect);
		PWidth = rect.right - rect.left;
		PHeight = rect.bottom - rect.top;
		break;

	case WM_GETMINMAXINFO:
		pmima = (MINMAXINFO*)lParam;
		pmima->ptMinTrackSize.x = 560;
		pmima->ptMinTrackSize.y = 420;
		break;

	case WM_MOUSEMOVE:
		if ((OverLetter = GetLetter(hwnd, LOWORD(lParam), HIWORD(lParam))) &&
			(LastOverLetter != OverLetter) && (!Sieg) && (Galgen < MAXGALG))
		{
			if (mytimer) KillTimer(hwnd, mytimer); mytimer = 0;
			if (!IsRectEmpty(&repaint))
			{
				InvalidateRect(hwnd, &repaint, TRUE);
				SetRect(&repaint, 0, 0, 0, 0);
			}
			HighLight(hwnd, OverLetter);
			PlaySound((wchar_t*)L"BLOB", hInst,
				SND_RESOURCE | SND_NOWAIT | SND_NOSTOP |
				SND_NODEFAULT | SND_ASYNC);
			mytimer = SetTimer(hwnd, IDM_TIMER, 1000, NULL);
		}
		LastOverLetter = OverLetter;
		break;

	case WM_LBUTTONDOWN:
		DownLetter = GetLetter(hwnd, LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_LBUTTONUP:
		UpLetter = GetLetter(hwnd, LOWORD(lParam), HIWORD(lParam));
		if ((UpLetter) && (UpLetter == DownLetter))
		{
			if ((Galgen < MAXGALG) && (!Sieg))
			{
				PressChar(hwnd, UpLetter);
			}
		}
		break;

	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}

