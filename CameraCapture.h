#pragma once

#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <cstdint>
#include <vector>
#include <string>

class CameraCapture
{
public:
    CameraCapture();
    ~CameraCapture();

    // Initializes Windows Media Foundation
    static bool InitializeMF();
    static void ShutdownMF();

    // Start capturing at requested resolution (targetWidth, targetHeight, targetFps)
    // Automatically attempts fallback to 640x480 if target resolution fails
    bool Start(int deviceIndex, int targetWidth, int targetHeight, int targetFps);
    void Stop();

    // Read next video frame (non-blocking / short timeout)
    // Produces clean, top-down 32-bit BGRA buffer (B=0, G=1, R=2, A=3)
    bool ReadFrame(std::vector<uint8_t>& outBgraBuffer, int& outWidth, int& outHeight);

    bool IsCapturing() const { return m_isCapturing; }
    int GetWidth() const { return m_actualWidth; }
    int GetHeight() const { return m_actualHeight; }
    bool IsBottomUp() const { return m_isBottomUp; }
    int GetStride() const { return m_stride; }
    std::wstring GetDeviceName() const { return m_deviceName; }
    std::string GetLastErrorMsg() const { return m_lastError; }

private:
    IMFSourceReader* m_pReader;
    bool m_isCapturing;
    int m_actualWidth;
    int m_actualHeight;
    int m_stride;
    bool m_isBottomUp;
    std::wstring m_deviceName;
    std::string m_lastError;
    GUID m_videoSubtype;

    bool ConfigureMediaType(int targetWidth, int targetHeight, int targetFps);
    static void ConvertYUY2ToBGRA(const uint8_t* yuy2, uint8_t* bgra, int width, int height, int stride, bool isBottomUp);
    static void ConvertNV12ToBGRA(const uint8_t* nv12, uint8_t* bgra, int width, int height, int stride, bool isBottomUp);
};
