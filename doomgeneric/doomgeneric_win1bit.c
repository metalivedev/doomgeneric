#include "doomkeys.h"

#include "doomgeneric.h"

#include <stdio.h>

#include <Windows.h>

static BITMAPINFO s_Bmi = { sizeof(BITMAPINFOHEADER), DOOMGENERIC_RESX, -DOOMGENERIC_RESY, 1, 32 };
static HWND s_Hwnd = 0;
static HDC s_Hdc = 0;


#define KEYQUEUE_SIZE 16

static unsigned short s_KeyQueue[KEYQUEUE_SIZE];
static unsigned int s_KeyQueueWriteIndex = 0;
static unsigned int s_KeyQueueReadIndex = 0;

// For a copy of the DG_ScreenBuffer. MUST be the same size.
uint32_t* bit_ScreenBuffer = 0;
size_t bit_ScreenBuffer_size = (DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

typedef enum {
	FLOYD,
	STUKI
} dither_t;

static unsigned char convertToDoomKey(unsigned char key)
{
	switch (key)
	{
	case VK_RETURN:
		key = KEY_ENTER;
		break;
	case VK_ESCAPE:
		key = KEY_ESCAPE;
		break;
	case VK_LEFT:
		key = KEY_LEFTARROW;
		break;
	case VK_RIGHT:
		key = KEY_RIGHTARROW;
		break;
	case VK_UP:
		key = KEY_UPARROW;
		break;
	case VK_DOWN:
		key = KEY_DOWNARROW;
		break;
	case VK_CONTROL:
		key = KEY_FIRE;
		break;
	case VK_SPACE:
		key = KEY_USE;
		break;
	case VK_SHIFT:
		key = KEY_RSHIFT;
		break;
	default:
		key = tolower(key);
		break;
	}

	return key;
}

static void addKeyToQueue(int pressed, unsigned char keyCode)
{
	unsigned char key = convertToDoomKey(keyCode);

	unsigned short keyData = (pressed << 8) | key;

	s_KeyQueue[s_KeyQueueWriteIndex] = keyData;
	s_KeyQueueWriteIndex++;
	s_KeyQueueWriteIndex %= KEYQUEUE_SIZE;
}

static LRESULT CALLBACK wndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		ExitProcess(0);
		break;
	case WM_KEYDOWN:
		addKeyToQueue(1, wParam);
		break;
	case WM_KEYUP:
		addKeyToQueue(0, wParam);
		break;
	default:
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void DG_Init()
{
	// window creation
	const char windowClassName[] = "DoomWindowClass";
	const char windowTitle[] = "Doom";
	WNDCLASSEXA wc;

	wc.cbSize = sizeof(WNDCLASSEXA);
	wc.style = 0;
	wc.lpfnWndProc = wndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = 0;
	wc.hIcon = 0;
	wc.hCursor = 0;
	wc.hbrBackground = 0;
	wc.lpszMenuName = 0;
	wc.lpszClassName = windowClassName;
	wc.hIconSm = 0;

	if (!RegisterClassExA(&wc))
	{
		printf("Window Registration Failed!");

		exit(-1);
	}

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = DOOMGENERIC_RESX;
	rect.bottom = DOOMGENERIC_RESY;
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

	HWND hwnd = CreateWindowExA(0, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, rect.right - rect.left, rect.bottom - rect.top, 0, 0, 0, 0);
	if (hwnd)
	{
		s_Hwnd = hwnd;

		s_Hdc = GetDC(hwnd);
		ShowWindow(hwnd, SW_SHOW);
	}
	else
	{
		printf("Window Creation Failed!");

		exit(-1);
	}

	memset(s_KeyQueue, 0, KEYQUEUE_SIZE * sizeof(unsigned short));

	// Allocate enough space for a copy of the buffer to make 1-bit
	// TODO Add error checking for failed malloc
	bit_ScreenBuffer = malloc(bit_ScreenBuffer_size);
}

#define PIXELOFF(x,y) (x + (y * DOOMGENERIC_RESX))
#define RGBA(b) ((b<<24) | (b<<16) | (b<<8) | b )
#define LOWB(n) (0xFF & n)

// assumes bit_ScreenBuffer is allocated and is sized by DOOMGENERIC_RES
void ditherPixel(unsigned int x, unsigned int y, int quanterror, int mult, int div) {
	unsigned int offset;
	int newpixel;
	offset = PIXELOFF(x, y);
	newpixel = LOWB(bit_ScreenBuffer[offset]) + quanterror * mult / div;
	bit_ScreenBuffer[offset] = RGBA(newpixel);
}

// Take the DG_Screenbuffer and dither it
void dither(dither_t dithering){
	unsigned int limit = 0;
	unsigned int x, y, offset;
	int oldpixel, newpixel;
	int quanterror;

	// TODO add error checking for failed memcpy
	memcpy_s(bit_ScreenBuffer, bit_ScreenBuffer_size, DG_ScreenBuffer, bit_ScreenBuffer_size);

	// The limit is needed so I don't exceed the boundaries of the image.
	switch (dithering) {
	case FLOYD:
		limit = 1;
		break;
	case STUKI:
		limit = 2;
		break;

	default:
		limit = 0;
	}

	/* Dither the array to so that each value is either 0 or 255.
		Details on the algorithm at https ://en.wikipedia.org/wiki/Floyd%E2%80%93Steinberg_dithering
		(Stuki dithering is very similar, but slightly more complicated.)
	*/

	for (y = limit; y < (DOOMGENERIC_RESY - limit); y++) {
		for (x = limit; x < (DOOMGENERIC_RESX - limit); x++) {
			offset = PIXELOFF(x,y);
			// It looks like the greyscale palette replicates the value for RGBA, so just look at one byte
			oldpixel = LOWB(bit_ScreenBuffer[offset]);

			newpixel = (oldpixel > 0x80) ? (char) 0xFF : (char) 0;
			bit_ScreenBuffer[offset] = RGBA(newpixel);
			quanterror = oldpixel - newpixel; // hmm, this is often going to be negative ... so unsigned doesn't work.

			if (dithering == FLOYD) {
				ditherPixel(x+1, y, quanterror, 7, 16);
				ditherPixel(x - 1, y + 1, quanterror, 3, 16);
				ditherPixel(x, y + 1, quanterror, 5, 16);
				ditherPixel(x + 1, y + 1, quanterror, 1, 16);
			}
			else if (dithering == STUKI) {
				ditherPixel(x + 1, y, quanterror, 8, 42);
				ditherPixel(x + 2, y, quanterror, 4, 42);
				ditherPixel(x - 2, y + 1, quanterror, 2, 42); 
				ditherPixel(x - 1, y + 1, quanterror, 4, 42);
				ditherPixel(x, y + 1, quanterror, 8, 42);
				ditherPixel(x + 1, y + 1, quanterror, 4, 42);
				ditherPixel(x + 2, y + 1, quanterror, 2, 42);
				ditherPixel(x - 2, y + 2, quanterror, 1, 42);
				ditherPixel(x - 1, y + 2, quanterror, 2, 42);
				ditherPixel(x, y + 2, quanterror, 4, 42);
				ditherPixel(x + 1, y + 2, quanterror, 2, 42);
				ditherPixel(x + 2, y + 2, quanterror, 1, 42);
			}
		}
	}
}

void DG_DrawFrame()
{
	MSG msg;
	memset(&msg, 0, sizeof(msg));

	while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}

	dither(STUKI);
	StretchDIBits(s_Hdc, 0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY, 0, 0, DOOMGENERIC_RESX, DOOMGENERIC_RESY, bit_ScreenBuffer, &s_Bmi, 0, SRCCOPY);

	SwapBuffers(s_Hdc);
}

void DG_SleepMs(uint32_t ms)
{
	Sleep(ms);
}

uint32_t DG_GetTicksMs()
{
	return GetTickCount();
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	if (s_KeyQueueReadIndex == s_KeyQueueWriteIndex)
	{
		//key queue is empty

		return 0;
	}
	else
	{
		unsigned short keyData = s_KeyQueue[s_KeyQueueReadIndex];
		s_KeyQueueReadIndex++;
		s_KeyQueueReadIndex %= KEYQUEUE_SIZE;

		*pressed = keyData >> 8;
		*doomKey = keyData & 0xFF;

		return 1;
	}
}

void DG_SetWindowTitle(const char * title)
{
	if (s_Hwnd)
	{
		SetWindowTextA(s_Hwnd, title);
	}
}