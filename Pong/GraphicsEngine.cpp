#include "GraphicsEngine.h"

#include <iostream>
#include <ostream>

#include "Pong.h"
#include "Server.h"
#include <fstream>

using namespace std;

HDC GraphicsEngine::hdc;
HWND GraphicsEngine::hwnd;

int GraphicsEngine::ScreenSize = 800;

HBITMAP GraphicsEngine::fontTexture = nullptr;
int GraphicsEngine::charWidth = 14;
int GraphicsEngine::charHeight = 18;
int GraphicsEngine::textureWidth = 256;
int GraphicsEngine::charsPerRow = textureWidth / charWidth; // 18 characters per row

// Function to handle window messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_PAINT: {
            // Get the device context for drawing
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);  // Begin painting

            // End painting and release the device context
            EndPaint(hwnd, &ps);
            return 0;
    }

    case WM_KEYDOWN: {
            
            // Detect key press
            if (Server::isServer)
            {
                if (wParam == 'W') {
                    Pong::wPressed = true;
                    Pong::sPressed = false;
                }
                else if (wParam == 'S') {
                    Pong::wPressed = false;
                    Pong::sPressed = true;
                }
                else
                {
                    Pong::wPressed = false;
                    Pong::sPressed = false;
                }
            }
            else
            {
                if (wParam == VK_UP) {
                    Pong::upPressed = true;
                    Pong::downPressed = false;
                }
                else if (wParam == VK_DOWN) {
                    Pong::upPressed = false;
                    Pong::downPressed = true;
                }
                else
                {
                    Pong::upPressed = false;
                    Pong::downPressed = false;
                }
            }

            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            }
            
            return 0;
    }
    case WM_KEYUP:
        {
            if (Server::isServer)
            {
                if (wParam == 'W') {
                    Pong::wPressed = false;
                }
                else if (wParam == 'S') {
                    Pong::sPressed = false;
                }
            }
            else
            {
                if (wParam == VK_UP) {
                    Pong::upPressed = false;
                }
                else if (wParam == VK_DOWN) {
                    Pong::downPressed = false;
                }
            }

            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void GraphicsEngine::CreateScreen() {
    // Register the window class
    const wchar_t* className = L"GraphicsClass";  // Use wide-character string (L"")
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = className;

    RegisterClass(&wc);

    // Create a window with styles that disable resizing
    DWORD windowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    LPCWSTR windowName;

    if (Server::isServer)
        windowName = L"Pong Game Server";
    else
        windowName = L"Pong Game Client";
    
    GraphicsEngine::hwnd = CreateWindowEx(
        0, className, windowName, windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, ScreenSize, ScreenSize, nullptr, nullptr,
        wc.hInstance, nullptr
    );

    if (hwnd == nullptr) {
        cout << "Error creating window" << endl;
    }

    ShowWindow(hwnd, SW_SHOW);

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);



    wchar_t path[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, path);
    std::wcout << L"Current Working Directory: " << path << std::endl;
    
    GraphicsEngine::LoadFontTexture(L"digits.bmp");
}

// Function to clear the screen (draw the white background)
void GraphicsEngine::ClearScreen(HDC hdcMem) {
    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    FillRect(hdcMem, &clientRect, (HBRUSH)GetStockObject(BLACK_BRUSH));
}

void GraphicsEngine::DrawRectangle(HDC hdcMem, const RECT& rect, COLORREF color) {
    SetDCBrushColor(hdcMem, color);
    FillRect(hdcMem, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
}

bool GraphicsEngine::DrawScreen() {
    // Handle messages (like input, window events)
    MSG msg = {};
    if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT)
            return false;  // Exit the game if the quit message is received

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Get the device context for the window
    hdc = GetDC(hwnd);

    // Create a memory DC and bitmap for double buffering
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, ScreenSize, ScreenSize);
    HGDIOBJ oldBitmap = SelectObject(hdcMem, hBitmap);

    // Fill the memory DC with white color (to clear the screen)
    ClearScreen(hdcMem);

    // Perform all your drawing operations to the memory DC
    DrawPaddles(hdcMem);
    DrawDivider(hdcMem);
    DrawBall(hdcMem);
    
    // Draw Score
    std::string player1ScoreText = std::to_string(Pong::player1Score);
    DrawString(hdcMem, player1ScoreText, ScreenSize / 4 - charWidth, 50, 2, 2);
    
    std::string player2ScoreText = std::to_string(Pong::player2Score);
    DrawString(hdcMem, player2ScoreText, 3 * ScreenSize / 4 - charWidth, 50, 2, 2);
    
    // Copy the memory DC contents to the screen (window)
    BitBlt(hdc, 0, 0, ScreenSize, ScreenSize, hdcMem, 0, 0, SRCCOPY);

    // Clean up
    SelectObject(hdcMem, oldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdc);

    return true;
}

void GraphicsEngine::DrawPaddles(HDC hdcMem) {
    
    RECT puck1Rect = { static_cast<LONG>(Pong::Puck1->posX) - Pong::Puck1->width, static_cast<LONG>(Pong::Puck1->posY) - Pong::Puck1->height, static_cast<LONG>(Pong::Puck1->posX) + Pong::Puck1->width, static_cast<LONG>(Pong::Puck1->posY + Pong::Puck1->height) };
    DrawRectangle(hdcMem, puck1Rect, RGB(255, 255, 255));
    
    RECT puck2Rect = { static_cast<LONG>(Pong::Puck2->posX) - Pong::Puck2->width, static_cast<LONG>(Pong::Puck2->posY) - Pong::Puck2->height, static_cast<LONG>(Pong::Puck2->posX) + Pong::Puck2->width, static_cast<LONG>(Pong::Puck2->posY + Pong::Puck2->height) };
    DrawRectangle(hdcMem, puck2Rect, RGB(255, 255, 255));
}

void GraphicsEngine::DrawDivider(HDC hdcMem) {

    for (int i = 0; i < 40; i += 2)
    {
        RECT divider = { ScreenSize/2 - 3, i * 25, ScreenSize/2 + 3, i * 25 + 20 };
        DrawRectangle(hdcMem, divider, RGB(255, 255, 255));
    }
}

void GraphicsEngine::DrawBall(HDC hdcMem) {
    
    RECT ballRect = { static_cast<LONG>(Pong::Ball->posX) - Pong::Ball->width, static_cast<LONG>(Pong::Ball->posY) - Pong::Ball->height, static_cast<LONG>(Pong::Ball->posX) + Pong::Ball->width, static_cast<LONG>(Pong::Ball->posY + Pong::Ball->height) };
    DrawRectangle(hdcMem, ballRect, RGB(255, 255, 255));
}

bool FileExists(const wchar_t* filename) {
    std::ifstream file(filename);
    return file.good();
}

HBITMAP GraphicsEngine::LoadBMPManual(const wchar_t* filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        std::wcerr << L"Error: Could not open BMP file: " << filename << std::endl;
        return nullptr;
    }

    BITMAPFILEHEADER fileHeader;
    file.read(reinterpret_cast<char*>(&fileHeader), sizeof(BITMAPFILEHEADER));

    if (fileHeader.bfType != 0x4D42) {
        std::cerr << "Error: Not a valid BMP file!" << std::endl;
        return nullptr;
    }

    BITMAPINFOHEADER infoHeader;
    file.read(reinterpret_cast<char*>(&infoHeader), sizeof(BITMAPINFOHEADER));

    if (infoHeader.biBitCount != 32) {
        std::cerr << "Error: BMP is not 32-bit! Convert it to 32-bit BMP with transparency." << std::endl;
        return nullptr;
    }

    int width = infoHeader.biWidth;
    int height = abs(infoHeader.biHeight);
    int bytesPerPixel = 4;
    int expectedSize = width * height * bytesPerPixel;

    if (infoHeader.biSizeImage == 0) {
        infoHeader.biSizeImage = expectedSize;
    }

    file.seekg(fileHeader.bfOffBits, std::ios::beg);

    BITMAPINFO bmpInfo = {};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = width;
    bmpInfo.bmiHeader.biHeight = height;
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 32;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    bmpInfo.bmiHeader.biSizeImage = infoHeader.biSizeImage;

    void* pixels;
    HBITMAP hBitmap = CreateDIBSection(NULL, &bmpInfo, DIB_RGB_COLORS, &pixels, NULL, 0);
    if (!hBitmap) {
        DWORD error = GetLastError();
        std::cerr << "Error: Failed to create DIB section! Error Code: " << error << std::endl;
        return nullptr;
    }

    // Read pixel data into DIB section
    file.read(reinterpret_cast<char*>(pixels), infoHeader.biSizeImage);
    file.close();

    // Ensure the alpha channel is properly set
    uint8_t* pixelData = static_cast<uint8_t*>(pixels);
    for (int i = 0; i < expectedSize; i += 4) {
        if (pixelData[i + 3] == 0) {  // If fully transparent, force transparency
            pixelData[i + 0] = 0;  // Blue
            pixelData[i + 1] = 0;  // Green
            pixelData[i + 2] = 0;  // Red
        }
    }

    return hBitmap;
}



void GraphicsEngine::LoadFontTexture(const wchar_t* filename) {

    if (!FileExists(filename)) {
        std::wcerr << L"Error: File not found -> " << filename << std::endl;
        return;
    }
    else
    {
        cout << "File found" << endl;
    }
    
    fontTexture = LoadBMPManual(filename);
    
    if (!fontTexture) {
        DWORD error = GetLastError();
        std::wcerr << L"Failed to load font texture: " << filename
                   << L" | Error Code: " << error << std::endl;
    } else {
        std::wcout << L"Font texture loaded successfully: " << filename << std::endl;
    }
}

void GraphicsEngine::DrawChar(HDC hdcMem, char c, int x, int y, int scaleX = 1, int scaleY = 1) {
    if (!fontTexture) return;

    int asciiIndex = c - 32;
    if (asciiIndex < 0 || asciiIndex >= (charsPerRow * (textureWidth / charHeight))) return;

    int col = asciiIndex % charsPerRow;
    int row = asciiIndex / charsPerRow;

    int srcX = col * charWidth;
    int srcY = row * charHeight;

    int destWidth = charWidth * scaleX;
    int destHeight = charHeight * scaleY;

    HDC hdcSprite = CreateCompatibleDC(hdcMem);
    HGDIOBJ oldBitmap = SelectObject(hdcSprite, fontTexture);

    StretchBlt(hdcMem, x, y, destWidth, destHeight, 
               hdcSprite, srcX, srcY, charWidth, charHeight, SRCCOPY);

    SelectObject(hdcSprite, oldBitmap);
    DeleteDC(hdcSprite);
}



void GraphicsEngine::DrawString(HDC hdcMem, const std::string& text, int x, int y, int scaleX = 1, int scaleY = 1) {
    int spacing = charWidth + 2; // Add a slight space between characters

    for (size_t i = 0; i < text.size(); i++) {
        DrawChar(hdcMem, text[i], x + (i * spacing), y, scaleX, scaleY);
    }
}
