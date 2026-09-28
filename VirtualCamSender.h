#pragma once

#include <cstdint>
#include <memory>
#include <string>

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

    // Send a 32-bit RGBA (or BGRA) frame to UnityCapture DirectShow filter
    bool SendFrame(int width, int height, const uint8_t* rgbaBuffer);

private:
    std::unique_ptr<SharedImageMemory> m_shm;
    int m_deviceIndex;
};
