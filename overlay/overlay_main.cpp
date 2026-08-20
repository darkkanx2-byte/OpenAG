#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <vector>
#include <cmath>
#pragma comment(lib, "gdiplus.lib")
#include "overlay_window.h"

// Shared Memory yapısı (client.dll ile aynı olmalı)
#define SHARED_MEMORY_NAME "OpenAG_ESP_SharedMemory"
#define SHARED_MEMORY_SIZE 8192

struct SharedPlayerData
{
    bool valid;
    float screenX, screenY;
    float distance;
    int health;
    int team;
    bool isEnemy;
    bool isDead;
    char name[32];
    float origin[3];
};

struct SharedRadarData
{
    float relativeX, relativeY;
    float yaw;
    int team;
    bool isEnemy;
    char name[32];
};

struct ESPSharedData
{
    bool valid;
    int playerCount;
    SharedPlayerData players[32];

    struct {
        float localOrigin[3];
        float localYaw;
        int playerCount;
        SharedRadarData radarPlayers[32];
    } radar;

    bool espEnabled;
    bool radarEnabled;
    bool enemiesOnly;
    int screenWidth;
    int screenHeight;
};

class ESPOverlay
{
public:
    ESPOverlay() : m_hWnd(NULL), m_hdcMemory(NULL), m_hBitmap(NULL), 
                   m_pGraphics(NULL), m_hMapFile(NULL), m_pBuf(NULL) {}

    ~ESPOverlay() { Shutdown(); }

    bool Initialize();
    void Shutdown();
    void Run();

private:
    HWND m_hWnd;
    HDC m_hdcMemory;
    HBITMAP m_hBitmap;
    Gdiplus::Graphics* m_pGraphics;

    HANDLE m_hMapFile;
    LPVOID m_pBuf;

    int m_ScreenWidth;
    int m_ScreenHeight;

    bool CreateOverlayWindow();
    void ReadSharedData(ESPSharedData& data);
    void Render(const ESPSharedData& data);
    void DrawESP(Gdiplus::Graphics* g, const ESPSharedData& data);
    void DrawRadar(Gdiplus::Graphics* g, const ESPSharedData& data);
    void Draw3DBox(Gdiplus::Graphics* g, float x, float y, float distance, int health, bool isEnemy);
    void DrawHealthBar(Gdiplus::Graphics* g, float x, float y, int health);

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
};

bool ESPOverlay::Initialize()
{
    Gdiplus::GdiplusStartupInput input;
    Gdiplus::GdiplusStartupOutput output;
    ULONG_PTR token;
    Gdiplus::GdiplusStartup(&token, &input, &output);

    // Shared Memory aç
    m_hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, SHARED_MEMORY_NAME);
    if (!m_hMapFile)
    {
        // İlk açılış - oluştur
        m_hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, SHARED_MEMORY_SIZE, SHARED_MEMORY_NAME);
        if (!m_hMapFile) return false;
    }

    m_pBuf = MapViewOfFile(m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, SHARED_MEMORY_SIZE);
    if (!m_pBuf) return false;

    // Ekran boyutları
    m_ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    m_ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    return CreateOverlayWindow();
}

bool ESPOverlay::CreateOverlayWindow()
{
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "OpenAG_ESP_Overlay";
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    RegisterClassEx(&wc);

    // Oyun penceresini bul
    HWND hGame = FindWindow("Valve001", NULL);
    if (!hGame) hGame = FindWindow(NULL, "Half-Life");

    RECT rc;
    if (hGame)
    {
        GetClientRect(hGame, &rc);
        ClientToScreen(hGame, (LPPOINT)&rc.left);
        ClientToScreen(hGame, (LPPOINT)&rc.right);
    }
    else
    {
        rc.left = 0; rc.top = 0;
        rc.right = m_ScreenWidth; rc.bottom = m_ScreenHeight;
    }

    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    m_hWnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "OpenAG_ESP_Overlay",
        "ESP Overlay",
        WS_POPUP,
        rc.left, rc.top, width, height,
        NULL, NULL, GetModuleHandle(NULL), NULL
    );

    if (!m_hWnd) return false;

    SetLayeredWindowAttributes(m_hWnd, RGB(0, 0, 0), 0, LWA_COLORKEY);

    HDC hdcScreen = GetDC(NULL);
    m_hdcMemory = CreateCompatibleDC(hdcScreen);
    m_hBitmap = CreateCompatibleBitmap(hdcScreen, width, height);
    SelectObject(m_hdcMemory, m_hBitmap);
    ReleaseDC(NULL, hdcScreen);

    m_pGraphics = new Gdiplus::Graphics(m_hdcMemory);
    m_pGraphics->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    m_pGraphics->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);

    ShowWindow(m_hWnd, SW_SHOW);

    return true;
}

void ESPOverlay::Shutdown()
{
    if (m_pGraphics) { delete m_pGraphics; m_pGraphics = NULL; }
    if (m_hBitmap) DeleteObject(m_hBitmap);
    if (m_hdcMemory) DeleteDC(m_hdcMemory);
    if (m_hWnd) DestroyWindow(m_hWnd);
    if (m_pBuf) UnmapViewOfFile(m_pBuf);
    if (m_hMapFile) CloseHandle(m_hMapFile);
}

void ESPOverlay::ReadSharedData(ESPSharedData& data)
{
    if (m_pBuf)
        memcpy(&data, m_pBuf, sizeof(ESPSharedData));
    else
        memset(&data, 0, sizeof(ESPSharedData));
}

void ESPOverlay::Run()
{
    MSG msg;
    ESPSharedData data;

    while (true)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) return;
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        ReadSharedData(data);
        Render(data);

        Sleep(1);  // ~1000 FPS limit
    }
}

void ESPOverlay::Render(const ESPSharedData& data)
{
    if (!m_hWnd || !m_pGraphics) return;

    RECT rc;
    GetClientRect(m_hWnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    // Arka planı siyah (transparent) yap
    FillRect(m_hdcMemory, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    if (data.valid)
    {
        if (data.espEnabled)
            DrawESP(m_pGraphics, data);

        if (data.radarEnabled)
            DrawRadar(m_pGraphics, data);
    }

    // Layered window güncelle
    HDC hdcScreen = GetDC(NULL);
    POINT ptSrc = {0, 0};
    SIZE size = {width, height};
    BLENDFUNCTION blend = {AC_SRC_OVER, 0, 255, AC_SRC_ALPHA};

    UpdateLayeredWindow(m_hWnd, hdcScreen, NULL, &size, m_hdcMemory, &ptSrc, RGB(0,0,0), &blend, ULW_COLORKEY);

    ReleaseDC(NULL, hdcScreen);
}

void ESPOverlay::DrawESP(Gdiplus::Graphics* g, const ESPSharedData& data)
{
    Gdiplus::SolidBrush textBrush(Gdiplus::Color(255, 255, 255));
    Gdiplus::Font font(L"Consolas", 9);

    for (int i = 0; i < data.playerCount && i < 32; i++)
    {
        const auto& p = data.players[i];

        if (!p.valid || p.isDead) continue;
        if (data.enemiesOnly && !p.isEnemy) continue;

        float x = p.screenX;
        float y = p.screenY;

        Gdiplus::Color color = p.isEnemy ? 
            Gdiplus::Color(255, 50, 50) : Gdiplus::Color(50, 255, 50);

        // Box boyutları (mesafeye göre)
        float boxHeight = 4000.0f / p.distance;
        if (boxHeight < 20) boxHeight = 20;
        if (boxHeight > 200) boxHeight = 200;
        float boxWidth = boxHeight / 2.2f;

        // 2D Box
        Gdiplus::Pen pen(color, 1.5f);
        g->DrawRectangle(&pen, x - boxWidth/2, y - boxHeight, boxWidth, boxHeight);

        // Köşe çizgileri (corner box)
        float cornerSize = boxWidth / 4;
        Gdiplus::Pen cornerPen(color, 2.0f);

        // Sol üst
        g->DrawLine(&cornerPen, x - boxWidth/2, y - boxHeight, x - boxWidth/2 + cornerSize, y - boxHeight);
        g->DrawLine(&cornerPen, x - boxWidth/2, y - boxHeight, x - boxWidth/2, y - boxHeight + cornerSize);

        // Sağ üst
        g->DrawLine(&cornerPen, x + boxWidth/2 - cornerSize, y - boxHeight, x + boxWidth/2, y - boxHeight);
        g->DrawLine(&cornerPen, x + boxWidth/2, y - boxHeight, x + boxWidth/2, y - boxHeight + cornerSize);

        // Sol alt
        g->DrawLine(&cornerPen, x - boxWidth/2, y, x - boxWidth/2 + cornerSize, y);
        g->DrawLine(&cornerPen, x - boxWidth/2, y - cornerSize, x - boxWidth/2, y);

        // Sağ alt
        g->DrawLine(&cornerPen, x + boxWidth/2 - cornerSize, y, x + boxWidth/2, y);
        g->DrawLine(&cornerPen, x + boxWidth/2, y - cornerSize, x + boxWidth/2, y);

        // İsim
        wchar_t wname[32];
        mbstowcs(wname, p.name, 32);
        Gdiplus::RectF nameRect;
        g->MeasureString(wname, -1, &font, Gdiplus::PointF(x - 50, y - boxHeight - 20), &nameRect);

        Gdiplus::SolidBrush bgBrush(Gdiplus::Color(150, 0, 0, 0));
        g->FillRectangle(&bgBrush, nameRect.X - 2, nameRect.Y - 2, nameRect.Width + 4, nameRect.Height + 4);
        g->DrawString(wname, -1, &font, Gdiplus::PointF(x - nameRect.Width/2, y - boxHeight - 20), &textBrush);

        // Sağlık barı (sol tarafta)
        int barWidth = 4;
        int barHeight = (int)boxHeight;
        int barX = (int)(x - boxWidth/2 - barWidth - 3);
        int barY = (int)(y - boxHeight);

        Gdiplus::SolidBrush barBg(Gdiplus::Color(200, 0, 0, 0));
        g->FillRectangle(&barBg, barX, barY, barWidth, barHeight);

        int health = p.health;
        if (health > 100) health = 100;
        if (health < 0) health = 0;

        int fillHeight = (int)(barHeight * (health / 100.0f));

        Gdiplus::Color healthColor;
        if (health > 60) healthColor = Gdiplus::Color(255, 0, 255, 0);
        else if (health > 30) healthColor = Gdiplus::Color(255, 255, 255, 0);
        else healthColor = Gdiplus::Color(255, 255, 0, 0);

        Gdiplus::SolidBrush healthBrush(healthColor);
        g->FillRectangle(&healthBrush, barX, barY + (barHeight - fillHeight), barWidth, fillHeight);

        // Mesafe
        wchar_t wdist[32];
        swprintf(wdist, L"%.0fm", p.distance / 39.37f);
        g->DrawString(wdist, -1, &font, Gdiplus::PointF(x - 15, y + 5), &textBrush);
    }
}

void ESPOverlay::DrawRadar(Gdiplus::Graphics* g, const ESPSharedData& data)
{
    int radarX = data.screenWidth - 170;
    int radarY = 20;
    int radarSize = 150;

    // Arka plan
    Gdiplus::SolidBrush bgBrush(Gdiplus::Color(180, 0, 0, 0));
    g->FillRectangle(&bgBrush, radarX, radarY, radarSize, radarSize);

    Gdiplus::Pen borderPen(Gdiplus::Color(255, 100, 100, 100), 2.0f);
    g->DrawRectangle(&borderPen, radarX, radarY, radarSize, radarSize);

    int centerX = radarX + radarSize / 2;
    int centerY = radarY + radarSize / 2;

    // Merkez çizgileri
    Gdiplus::Pen crossPen(Gdiplus::Color(100, 50, 50, 50), 1.0f);
    g->DrawLine(&crossPen, centerX, radarY, centerX, radarY + radarSize);
    g->DrawLine(&crossPen, radarX, centerY, radarX + radarSize, centerY);

    // Yerel oyuncu (üçgen)
    Gdiplus::SolidBrush localBrush(Gdiplus::Color(255, 255, 255));
    Gdiplus::Point localPoints[3] = {
        Gdiplus::Point(centerX, centerY - 6),
        Gdiplus::Point(centerX - 4, centerY + 3),
        Gdiplus::Point(centerX + 4, centerY + 3)
    };
    g->FillPolygon(&localBrush, localPoints, 3);

    // Diğer oyuncular
    for (int i = 0; i < data.radar.playerCount && i < 32; i++)
    {
        const auto& rp = data.radar.radarPlayers[i];

        float rx = centerX + rp.relativeX * (radarSize / 2.0f);
        float ry = centerY - rp.relativeY * (radarSize / 2.0f);

        Gdiplus::Color color = rp.isEnemy ? 
            Gdiplus::Color(255, 255, 50, 50) : Gdiplus::Color(255, 50, 255, 50);

        Gdiplus::SolidBrush playerBrush(color);
        g->FillEllipse(&playerBrush, rx - 3, ry - 3, 6, 6);

        // Yön oku
        float yaw = rp.yaw * (3.14159f / 180.0f);
        float endX = rx + 8 * sinf(yaw);
        float endY = ry - 8 * cosf(yaw);

        Gdiplus::Pen dirPen(color, 1.5f);
        g->DrawLine(&dirPen, rx, ry, endX, endY);
    }
}

LRESULT CALLBACK ESPOverlay::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    ESPOverlay overlay;

    if (!overlay.Initialize())
    {
        MessageBox(NULL, "Overlay başlatılamadı!", "Hata", MB_OK | MB_ICONERROR);
        return 1;
    }

    overlay.Run();

    return 0;
}
