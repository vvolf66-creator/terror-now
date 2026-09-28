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

    // Initializes Windows Media Foundation and enumerates cameras
    static bool InitializeMF();
    static void ShutdownMF();

    // Start capturing at requested resolution (targetWidth, targetHeight, targetFps)
    bool Start(int deviceIndex, int targetWidth, int targetHeight, int targetFps);
    void Stop();

    // Read next video frame (non-blocking / short timeout)
    // Returns true if a new frame was decoded into outRgbaBuffer
    bool ReadFrame(std::vector<uint8_t>& outRgbaBuffer, int& outWidth, int& outHeight);

    bool IsCapturing() const { return m_isCapturing; }
    int GetWidth() const { return m_actualWidth; }
    int GetHeight() const { return m_actualHeight; }
    std::wstring GetDeviceName() const { return m_deviceName; }

private:
    IMFSourceReader* m_pReader;
    bool m_isCapturing;
    int m_actualWidth;
    int m_actualHeight;
    std::wstring m_deviceName;
    GUID m_videoSubtype;

    bool ConfigureMediaType(int targetWidth, int targetHeight, int targetFps);
    static void ConvertYUY2ToRGBA(const uint8_t* yuy2, uint8_t* rgba, int width, int height);
    static void ConvertNV12ToRGBA(const uint8_t* nv12, uint8_t* rgba, int width, int height);
};
