#pragma once
#include <string>
#include <windows.h>

class GraphicsEngine
{
public:
    static HDC hdc;
    static HWND hwnd;

    static int ScreenSize;

    static HBITMAP fontTexture;
    static int charWidth;
    static int charHeight;
    static int textureWidth;
    static int charsPerRow;
public:
    static bool DrawScreen(int remainingTime);
    static void CreateScreen();
    static void ClearScreen(HDC hdcMem);

    static void DrawRectangle(HDC hdcMem, const RECT& rect, COLORREF color);
    static void DrawPaddles(HDC hdcMem);
    static void DrawDivider(HDC hdcMem);
    static void DrawBall(HDC hdcMem);
    static void DrawScore(HDC hdcMem);
    static void DrawEndScore(HDC hdcMem);

    static void LoadFontTexture(const wchar_t* filename);
    static void DrawChar(HDC hdcMem, char c, int x, int y, int scaleX, int scaleY);
    static void DrawString(HDC hdcMem, const std::string& text, int x, int y, int scaleX, int scaleY);
    static int GetStringSize(std::string s);

    static HBITMAP LoadBMPManual(const wchar_t* filename);
};


