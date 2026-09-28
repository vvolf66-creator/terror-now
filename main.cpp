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
#include "DiagnosticEffect.h"

// Hotkey IDs
#define HOTKEY_ID_F8 9001
#define HOTKEY_ID_F9 9002

enum class AppMode
{
    NORMAL,
    DIAGNOSTIC
};

// Global Application State
static HWND g_hWnd = NULL;
static bool g_isRunning = true;
static AppMode g_mode = AppMode::NORMAL;
static std::chrono::steady_clock::time_point g_diagnosticStartTime;
static const int DIAGNOSTIC_DURATION_MS = 450;

static CameraCapture g_camera;
static VirtualCamSender g_virtualCam;
static DiagnosticEffect g_effect;

static std::vector<uint8_t> g_rawFrameBuffer;
static std::vector<uint8_t> g_processedFrameBuffer;
static int g_currentWidth = 1280;
static int g_currentHeight = 720;
static int g_targetFps = 30;

// Telemetry & FPS stats
static int g_renderedFrames = 0;
static double g_measuredFps = 30.0;
static auto g_lastFpsTime = std::chrono::steady_clock::now();

void SetModeNormal()
{
    g_mode = AppMode::NORMAL;
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void SetModeDiagnostic()
{
    g_mode = AppMode::DIAGNOSTIC;
    g_diagnosticStartTime = std::chrono::steady_clock::now();
    InvalidateRect(g_hWnd, NULL, FALSE);
}

void SwitchResolution(int w, int h)
{
    g_currentWidth = w;
    g_currentHeight = h;
    g_camera.Stop();
    g_camera.Start(0, g_currentWidth, g_currentHeight, g_targetFps);
    g_currentWidth = g_camera.GetWidth();
    g_currentHeight = g_camera.GetHeight();
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_HOTKEY:
        if (wParam == HOTKEY_ID_F8)
        {
            SetModeDiagnostic();
        }
        else if (wParam == HOTKEY_ID_F9)
        {
            SetModeNormal();
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE || wParam == 'Q')
        {
            PostQuitMessage(0);
        }
        else if (wParam == 'R')
        {
            // Toggle between 720p and 480p fallback
            if (g_currentWidth == 1280)
                SwitchResolution(640, 480);
            else
                SwitchResolution(1280, 720);
        }
        else if (wParam == VK_F8)
        {
            SetModeDiagnostic();
        }
        else if (wParam == VK_F9)
        {
            SetModeNormal();
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

        // Draw camera frame if available
        if (!g_processedFrameBuffer.empty() && g_currentWidth > 0 && g_currentHeight > 0)
        {
            BITMAPINFO bmi = { 0 };
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = g_currentWidth;
            bmi.bmiHeader.biHeight = -g_currentHeight; // top-down DIB
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            SetStretchBltMode(hdc, COLORONCOLOR);
            StretchDIBits(
                hdc,
                0, 0, clientW, clientH,
                0, 0, g_currentWidth, g_currentHeight,
                g_processedFrameBuffer.data(),
                &bmi,
                DIB_RGB_COLORS,
                SRCCOPY
            );
        }
        else
        {
            // Background fill if waiting for camera
            HBRUSH hBr = CreateSolidBrush(RGB(15, 17, 23));
            FillRect(hdc, &clientRect, hBr);
            DeleteObject(hBr);
        }

        // Draw HUD overlay
        SetBkMode(hdc, TRANSPARENT);
        HFONT hFont = CreateFontA(
            17, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI"
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);

        // HUD Banner Background
        RECT hudRect = { 10, 10, 480, 110 };
        HBRUSH hudBg = CreateSolidBrush(RGB(20, 24, 33));
        FillRect(hdc, &hudRect, hudBg);
        DeleteObject(hudBg);
        FrameRect(hdc, &hudRect, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

        // Mode Status
        if (g_mode == AppMode::NORMAL)
        {
            SetTextColor(hdc, RGB(52, 211, 153)); // Emerald Green
            TextOutA(hdc, 20, 18, "MODE: NORMAL (LIVE PASSTHROUGH)", 31);
        }
        else
        {
            SetTextColor(hdc, RGB(248, 113, 113)); // Red Alert
            TextOutA(hdc, 20, 18, "MODE: DIAGNOSTIC TEST PULSE (450ms)", 35);
        }

        // Resolution & FPS
        SetTextColor(hdc, RGB(220, 225, 235));
        std::stringstream ss;
        ss << "Camera: " << g_currentWidth << "x" << g_currentHeight
           << " | Measured FPS: " << std::fixed << std::setprecision(1) << g_measuredFps;
        std::string statsStr = ss.str();
        TextOutA(hdc, 20, 42, statsStr.c_str(), (int)statsStr.length());

        // Virtual Camera & Chrome Status
        bool vcamActive = g_virtualCam.IsConnected();
        if (vcamActive)
        {
            SetTextColor(hdc, RGB(96, 165, 250)); // Blue
            TextOutA(hdc, 20, 64, "Virtual Camera: 'UnityCapture' (Chrome Connected)", 49);
        }
        else
        {
            SetTextColor(hdc, RGB(156, 163, 175)); // Gray
            TextOutA(hdc, 20, 64, "Virtual Camera: 'UnityCapture' (Waiting for Chrome)", 51);
        }

        // Hotkey Help
        SetTextColor(hdc, RGB(251, 191, 36)); // Amber
        TextOutA(hdc, 20, 86, "[F8] Diagnostic Pulse  |  [F9] Restore Normal  |  [R] Toggle Res", 64);

        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);

        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1; // Prevent flicker

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
        "ScareCam - Windows 10 Virtual Camera Engine (v1.0)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        960, 580,
        NULL, NULL, hInstance, NULL
    );

    if (!g_hWnd)
    {
        CameraCapture::ShutdownMF();
        return 1;
    }

    // 4. Register Global Hotkeys (F8 and F9 - work even when Chrome has foreground focus)
    RegisterHotKey(g_hWnd, HOTKEY_ID_F8, 0, VK_F8);
    RegisterHotKey(g_hWnd, HOTKEY_ID_F9, 0, VK_F9);

    // 5. Connect to UnityCapture Virtual Camera
    g_virtualCam.Initialize(0);

    // 6. Start Camera Capture at 1280x720 @ 30fps
    if (!g_camera.Start(0, g_currentWidth, g_currentHeight, g_targetFps))
    {
        // Try fallback to 640x480
        if (!g_camera.Start(0, 640, 480, 30))
        {
            MessageBoxA(
                g_hWnd,
                "Could not open physical webcam.\nEnsure no other app is using it and privacy settings allow camera access.",
                "ScareCam Camera Error",
                MB_ICONEXCLAMATION
            );
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

        // Auto-revert diagnostic mode after 450 ms
        if (g_mode == AppMode::DIAGNOSTIC)
        {
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - g_diagnosticStartTime).count();
            if (elapsed >= DIAGNOSTIC_DURATION_MS)
            {
                SetModeNormal();
            }
        }

        // Read physical webcam frame
        int w = 0, h = 0;
        if (g_camera.ReadFrame(g_rawFrameBuffer, w, h))
        {
            g_currentWidth = w;
            g_currentHeight = h;

            size_t frameBytes = (size_t)w * h * 4;
            if (g_processedFrameBuffer.size() != frameBytes)
            {
                g_processedFrameBuffer.resize(frameBytes);
            }

            // Apply diagnostic pulse if in diagnostic mode
            if (g_mode == AppMode::DIAGNOSTIC)
            {
                g_effect.Apply(g_rawFrameBuffer.data(), g_processedFrameBuffer.data(), w, h);
            }
            else
            {
                memcpy(g_processedFrameBuffer.data(), g_rawFrameBuffer.data(), frameBytes);
            }

            // Transmit to DirectShow Virtual Camera (for Chrome to receive)
            g_virtualCam.SendFrame(w, h, g_processedFrameBuffer.data());

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
    UnregisterHotKey(g_hWnd, HOTKEY_ID_F8);
    UnregisterHotKey(g_hWnd, HOTKEY_ID_F9);

    g_camera.Stop();
    g_virtualCam.Shutdown();
    CameraCapture::ShutdownMF();

    return (int)msg.wParam;
}
