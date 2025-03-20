#pragma once
#include <chrono>
#include <utility>
#include <windows.h>

class GraphicsEngine
{
public:
    static HDC hdc;
    static HWND hwnd;

    static int ScreenSize;

    static inline std::chrono::time_point last_update_time = std::chrono::high_resolution_clock::now();

    static HBITMAP fontTexture; 
    static int charWidth;
    static int charHeight;
    static int textureWidth;
    static int charsPerRow;
public:
    static void DrawRectangle(HDC hdcMem, const RECT& rect, COLORREF color);
    static bool DrawScreen();
    static void DrawPaddles(HDC hdcMem);
    static void DrawDivider(HDC hdcMem);
    static void DrawBall(HDC hdcMem);

    static void CreateScreen();
    static void ClearScreen(HDC hdcMem);

    static void LoadFontTexture(const wchar_t* filename);
    static void DrawChar(HDC hdcMem, char c, int x, int y, int scaleX, int scaleY);
    static void DrawString(HDC hdcMem, const std::string& text, int x, int y, int scaleX, int scaleY);

    static HBITMAP LoadBMPManual(const wchar_t* filename);

};
