#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

struct SharedImageMemory;

class VirtualCamSender
{
public:
    VirtualCamSender();
    ~VirtualCamSender();

    // Initializes connection to UnityCapture device instance (default index 0: "UnityCapture")
    bool Initialize(int deviceIndex = 0);
    void Shutdown();

    // Check if Chrome / receiving app has opened the virtual camera
    bool IsConnected();

    // Send a 32-bit RGBA frame to UnityCapture DirectShow filter
    bool SendFrame(int width, int height, const uint8_t* rgbaBuffer);

    // Send a 32-bit BGRA frame (converts in-place/buffered to RGBA for UnityCapture)
    bool SendFrameBGRA(int width, int height, const uint8_t* bgraBuffer);

private:
    std::unique_ptr<SharedImageMemory> m_shm;
    int m_deviceIndex;
    std::vector<uint8_t> m_conversionBuffer;
};
