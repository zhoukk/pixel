#include "pixel.h"
#include "opengl.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

#define WINDOWNAME "pixel"
#define CLASSNAME "WINDOW"
#define WINDOWSTYLE (WS_CAPTION/* | WS_POPUP */| WS_MINIMIZEBOX | WS_SYSMENU)

static DWORD g_frame_start = 0;
static char title_buff[256] = { 0 };
static HGLRC glrc;
static int width = 960;
static int height = 640;
static const char *start;

static void set_pixel_format(HDC dc) {
	int color_deep;
	int pixel_format;
	PIXELFORMATDESCRIPTOR pfd;

	color_deep = GetDeviceCaps(dc, BITSPIXEL);
	memset(&pfd, 0, sizeof(pfd));
	pfd.nSize = sizeof(pfd);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = color_deep;
	pfd.cDepthBits = 0;
	pfd.cStencilBits = 0;
	pfd.iLayerType = PFD_MAIN_PLANE;
	pixel_format = ChoosePixelFormat(dc, &pfd);
	SetPixelFormat(dc, pixel_format, &pfd);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_CREATE:
		do {
			GLenum err;
			HDC dc = GetDC(hWnd);
			set_pixel_format(dc);
			glrc = wglCreateContext(dc);
			if (glrc == 0) {
				fprintf(stderr, "wglCreateContext err : %d\n", (int)GetLastError());
				exit(1);
			}
			wglMakeCurrent(dc, glrc);
			err = glewInit();
			if (GLEW_OK != err) {
				fprintf(stderr, "glewInit err : %s\n", (char *)glewGetErrorString(err));
				exit(1);
			}
			fprintf(stdout, "GL_VERSION:%s\n", glGetString(GL_VERSION));
			fprintf(stdout, "GL_VENDOR:%s\n", glGetString(GL_VENDOR));
			fprintf(stdout, "GL_RENDERER:%s\n", glGetString(GL_RENDERER));
			fprintf(stdout, "GL_SHADING_LANGUAGE_VERSION:%s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
			pixel_start(width, height, start);
			ReleaseDC(hWnd, dc);
			SetTimer(hWnd, 0, 10, NULL);
		} while (0);
		break;
	case WM_SIZE:
		break;
	case WM_PAINT:
		if (GetUpdateRect(hWnd, NULL, FALSE)) {
			HDC dc = GetDC(hWnd);
			float elapsed = 0;
			DWORD cur = GetTickCount();
			if (g_frame_start > 0) {
				elapsed = (cur - g_frame_start) / 1000.0f;
			}
			g_frame_start = cur;
			pixel_frame(elapsed);
			SwapBuffers(dc);
			ValidateRect(hWnd, NULL);
			ReleaseDC(hWnd, dc);
		}
		return 0;
	case WM_TIMER:
		pixel_update(0.01f);
		InvalidateRect(hWnd, NULL, FALSE);
		snprintf(title_buff, 256, "pixel fps:%d", pixel_fps(0));
		SetWindowText(hWnd, (LPCSTR)title_buff);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		pixel_close();
		wglMakeCurrent(NULL, NULL);
		wglDeleteContext(glrc);
		return 0;
	case WM_LBUTTONDOWN:
		pixel_touch(0, TOUCH_BEGIN, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_LBUTTONUP:
		pixel_touch(0, TOUCH_END, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEMOVE:
		pixel_touch(0, TOUCH_MOVE, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_MOUSEWHEEL:
		pixel_wheel(GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA, LOWORD(lParam), HIWORD(lParam));
		break;
	case WM_KEYDOWN:
		pixel_key(KEY_DOWN, wParam);
		break;
	case WM_KEYUP:
		pixel_key(KEY_UP, wParam);
		break;
	case WM_SETFOCUS:
		pixel_resume();
		break;
	case WM_KILLFOCUS:
		pixel_pause();
		break;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}

int main(int argc, char *argv[]) {
	WNDCLASSEX wndcl;
	HWND hWnd;
	RECT rect;
	MSG msg;

	if (argc < 2) {
		fprintf(stderr, "usage: pixel.exe example.lua");
		return 0;
	}
	start = argv[1];
	if (argc > 3) {
		width = atoi(argv[2]);
		height = atoi(argv[3]);
	}

	wndcl.cbSize = sizeof(wndcl);
	wndcl.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndcl.lpfnWndProc = WndProc;
	wndcl.cbClsExtra = 0;
	wndcl.cbWndExtra = 0;
	wndcl.hInstance = GetModuleHandle(0);
	wndcl.hIcon = LoadImage(NULL, "pixel.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	wndcl.hIconSm = LoadImage(NULL, "pixel.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	wndcl.hCursor = LoadCursorFromFile("pixel.ani");
	wndcl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wndcl.lpszMenuName = NULL;
	wndcl.lpszClassName = CLASSNAME;

	if (!RegisterClassEx(&wndcl))
		return -1;
	rect.left = 0;
	rect.top = 0;
	rect.right = width;
	rect.bottom = height;
	AdjustWindowRect(&rect, WINDOWSTYLE, FALSE);

	hWnd = CreateWindowEx(0, CLASSNAME, WINDOWNAME, WINDOWSTYLE,
		(GetSystemMetrics(SM_CXSCREEN) - rect.right) / 2,
		(GetSystemMetrics(SM_CYSCREEN) - rect.bottom) / 2,
		rect.right - rect.left, rect.bottom - rect.top,
		HWND_DESKTOP, NULL, GetModuleHandle(0), NULL);

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnregisterClass(CLASSNAME, GetModuleHandle(0));
	return msg.wParam;
}
