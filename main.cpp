#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <chrono>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include "CameraCapture.h"
#include "VirtualCamSender.h"
#include "HorrorEffects.h"

// Hotkey IDs
#define HOTKEY_ID_F6 9001
#define HOTKEY_ID_F7 9002
#define HOTKEY_ID_F8 9003
#define HOTKEY_ID_F9 9004

// Global Application State
static HWND g_hWnd = NULL;
static bool g_isRunning = true;

static CameraCapture g_camera;
static VirtualCamSender g_virtualCam;
static HorrorEffects g_horror;

static std::vector<uint8_t> g_rawBgraBuffer;
static std::vector<uint8_t> g_processedBgraBuffer;
static int g_currentWidth = 1280;
static int g_currentHeight = 720;
static int g_targetFps = 30;

// Telemetry & FPS stats
static int g_renderedFrames = 0;
static double g_measuredFps = 30.0;
static auto g_lastFpsTime = std::chrono::steady_clock::now();

// Hotkey registration status
static bool g_f6Registered = false;
static bool g_f7Registered = false;
static bool g_f8Registered = false;
static bool g_f9Registered = false;

void TriggerEffect1()
{
    g_horror.TriggerEffect(HorrorEffectType::EDGE_LURKER);
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void TriggerEffect2()
{
    g_horror.TriggerEffect(HorrorEffectType::SUDDEN_LUNGER);
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void TriggerEffect3()
{
    g_horror.TriggerEffect(HorrorEffectType::HAUNTED_ANOMALY);
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void EmergencyRestoreNormal()
{
    g_horror.RestoreNormal();
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void SwitchResolution(int w, int h)
{
    g_currentWidth = w;
    g_currentHeight = h;
    g_camera.Stop();
    if (!g_camera.Start(0, g_currentWidth, g_currentHeight, g_targetFps))
    {
        // Fallback if requested resolution failed
        g_camera.Start(0, 640, 480, 30);
    }
    g_currentWidth = g_camera.GetWidth();
    g_currentHeight = g_camera.GetHeight();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID_F6)
        {
            TriggerEffect1();
        }
        else if (wParam == HOTKEY_ID_F7)
        {
            TriggerEffect2();
        }
        else if (wParam == HOTKEY_ID_F8)
        {
            TriggerEffect3();
        }
        else if (wParam == HOTKEY_ID_F9)
        {
            EmergencyRestoreNormal();
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == 'Q')
        {
            PostQuitMessage(0);
        }
        else if (wParam == '1' || wParam == VK_F6)
        {
            TriggerEffect1();
        }
        else if (wParam == '2' || wParam == VK_F7)
        {
            TriggerEffect2();
        }
        else if (wParam == '3' || wParam == VK_F8)
        {
            TriggerEffect3();
        }
        else if (wParam == '0' || wParam == '9' || wParam == 'N' || wParam == VK_F9)
        {
            EmergencyRestoreNormal();
        }
        else if (wParam == 'R')
        {
            // Toggle between 1280x720 and 640x480 fallback
            if (g_currentWidth == 1280)
                SwitchResolution(640, 480);
            else
                SwitchResolution(1280, 720);
        }
        break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        RECT clientRect;
        GetClientRect(hWnd, &clientRect);
        int clientW = clientRect.right - clientRect.left;
        int clientH = clientRect.bottom - clientRect.top;

        // Render live camera frame with correct BGRA color ordering
        if (!g_processedBgraBuffer.empty() && g_currentWidth > 0 && g_currentHeight > 0)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = g_currentWidth;
            bmi.bmiHeader.biHeight = -g_currentHeight; // Top-down DIB
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB; // BGRA format matches GDI

            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchDIBits(
                hdc,
                0, 0, clientW, clientH,
                0, 0, g_currentWidth, g_currentHeight,
                g_processedBgraBuffer.data(),
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );
        }
        else
        {
            // Dark solid background if waiting for camera
            HBRUSH hBr = CreateSolidBrush(RGB(12, 14, 18));
            FillRect(hdc, &clientRect, hBr);
            DeleteObject(hBr);
        }

        // Draw HUD overlay
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFontA(
            16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        // HUD Banner Background
        RECT hudRect = { 10, 10, 520, 135 };
        HBRUSH hudBg = CreateSolidBrush(RGB(18, 22, 30));
        FillRect(hdc, &hudRect, hudBg);
        DeleteObject(hudBg);
        FrameRect(hdc, &hudRect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

        // Line 1: Mode Status
        if (!g_horror.IsActive())
        {
            SetTextColor(hdc, RGB(52, 211, 153)); // Emerald Green
            TextOutA(hdc, 20, 18, "MODE: NORMAL (LIVE WEBCAM PASSTHROUGH)", 38);
        }
        else
        {
            SetTextColor(hdc, RGB(248, 113, 113)); // Red Alert
            std::string name = g_horror.GetCurrentEffectName();
            int pct = (int)(g_horror.GetProgress() * 100.0f);
            std::stringstream ss;
            ss << name << " [" << pct << "%]";
            std::string s = ss.str();
            TextOutA(hdc, 20, 18, s.c_str(), (int)s.length());
        }

        // Line 2: Camera Telemetry & Format
        SetTextColor(hdc, RGB(225, 230, 240));
        std::stringstream ssStats;
        ssStats << "Camera: " << g_currentWidth << "x" << g_currentHeight
                << " | FPS: " << std::fixed << std::setprecision(1) << g_measuredFps
                << " | Color: DirectShow BGRA -> Chrome RGBA";
        std::string statsStr = ssStats.str();
        TextOutA(hdc, 20, 42, statsStr.c_str(), (int)statsStr.length());

        // Line 3: Virtual Camera & Chrome Status
        bool vcamActive = g_virtualCam.IsConnected();
        if (vcamActive)
        {
            SetTextColor(hdc, RGB(96, 165, 250)); // Bright Blue
            TextOutA(hdc, 20, 64, "Virtual Camera: 'UnityCapture' (CONNECTED & TRANSMITTING)", 56);
        }
        else
        {
            SetTextColor(hdc, RGB(160, 165, 175)); // Muted Gray
            TextOutA(hdc, 20, 64, "Virtual Camera: 'UnityCapture' (Ready, waiting for Chrome)", 57);
        }

        // Line 4: Hotkey Commands
        SetTextColor(hdc, RGB(251, 191, 36)); // Amber
        TextOutA(hdc, 20, 86, "[F6] Edge Lurker  |  [F7] Sudden Lunger  |  [F8] Haunted Room", 60);

        // Line 5: Restoration & Utilities
        SetTextColor(hdc, RGB(147, 197, 253)); // Soft Cyan
        TextOutA(hdc, 20, 108, "[F9] RESTORE NORMAL IMMEDIATELY  |  [R] 720p/480p  |  [ESC] Exit", 64);

        // Hotkey Warning if any failed
        if (!g_f6Registered || !g_f7Registered || !g_f8Registered || !g_f9Registered)
        {
            SetTextColor(hdc, RGB(252, 165, 165));
            TextOutA(hdc, 20, 130, "Note: Some global hotkeys in use by OS; use number keys 1, 2, 3, 9 in window.", 76);
        }

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent flicker during frame drawing

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    // 1. Initialize Windows Media Foundation
    if (!CameraCapture::InitializeMF())
    {
        MessageBoxA(NULL, "Failed to initialize Windows Media Foundation.", "ScareCam Error", MB_ICONERROR);
        return 1;
    }

    // 2. Register Window Class
    WNDCLASSEXA wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "ScareCamWindowClass";

    if (!RegisterClassExA(&wc))
    {
        CameraCapture::ShutdownMF();
        return 1;
    }

    // 3. Create Main Window
    g_hWnd = CreateWindowExA(
        0,
        "ScareCamWindowClass",
        "ScareCam - Windows 10 Native Virtual Camera Engine (v1.0)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        980, 600,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd)
    {
        CameraCapture::ShutdownMF();
        return 1;
    }

    // 4. Register Global Hotkeys (F6, F7, F8, F9 - active even when Chrome has foreground focus)
    g_f6Registered = (RegisterHotKey(g_hWnd, HOTKEY_ID_F6, 0, VK_F6) != FALSE);
    g_f7Registered = (RegisterHotKey(g_hWnd, HOTKEY_ID_F7, 0, VK_F7) != FALSE);
    g_f8Registered = (RegisterHotKey(g_hWnd, HOTKEY_ID_F8, 0, VK_F8) != FALSE);
    g_f9Registered = (RegisterHotKey(g_hWnd, HOTKEY_ID_F9, 0, VK_F9) != FALSE);

    // 5. Connect to UnityCapture Virtual Camera
    g_virtualCam.Initialize(0);

    // 6. Start Camera Capture at 1280x720 @ 30fps
    if (!g_camera.Start(0, g_currentWidth, g_currentHeight, g_targetFps))
    {
        // Try fallback to 640x480
        if (!g_camera.Start(0, 640, 480, 30))
        {
            std::string err = g_camera.GetLastErrorMsg();
            if (err.empty()) err = "Could not open physical webcam.\nEnsure no other app is using it and privacy settings allow camera access.";
            MessageBoxA(g_hWnd, err.c_str(), "ScareCam Camera Warning", MB_ICONEXCLAMATION);
        }
    }

    g_currentWidth = g_camera.GetWidth();
    g_currentHeight = g_camera.GetHeight();

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);

    // 7. High-Performance Frame Loop with Message Dispatch
    MSG msg;
    while (g_isRunning)
    {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                g_isRunning = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!g_isRunning) break;

        // Read physical webcam frame (in canonical clean BGRA)
        int w = 0, h = 0;
        if (g_camera.ReadFrame(g_rawBgraBuffer, w, h))
        {
            g_currentWidth = w;
            g_currentHeight = h;

            size_t frameBytes = (size_t)w * h * 4;
            if (g_processedBgraBuffer.size() != frameBytes)
            {
                g_processedBgraBuffer.resize(frameBytes);
            }

            // Process active horror effect (or pass through cleanly)
            g_horror.ProcessFrame(g_rawBgraBuffer.data(), g_processedBgraBuffer.data(), w, h);

            // Transmit to DirectShow Virtual Camera for Chrome (converts BGRA -> RGBA)
            g_virtualCam.SendFrameBGRA(w, h, g_processedBgraBuffer.data());

            // FPS calculation
            g_renderedFrames++;
            auto now = std::chrono::steady_clock::now();
            auto fpsElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_lastFpsTime).count();
            if (fpsElapsed >= 1000)
            {
                g_measuredFps = (g_renderedFrames * 1000.0) / fpsElapsed;
                g_renderedFrames = 0;
                g_lastFpsTime = now;
            }

            // Redraw window preview
            InvalidateRect(g_hWnd, NULL, FALSE);
        }
        else
        {
            // Yield CPU slice if no new frame is ready
            Sleep(1);
        }
    }

    // 8. Clean Resource Teardown on Exit
    if (g_f6Registered) UnregisterHotKey(g_hWnd, HOTKEY_ID_F6);
    if (g_f7Registered) UnregisterHotKey(g_hWnd, HOTKEY_ID_F7);
    if (g_f8Registered) UnregisterHotKey(g_hWnd, HOTKEY_ID_F8);
    if (g_f9Registered) UnregisterHotKey(g_hWnd, HOTKEY_ID_F9);

    g_camera.Stop();
    g_virtualCam.Shutdown();
    CameraCapture::ShutdownMF();

    return (int)msg.wParam;
}
