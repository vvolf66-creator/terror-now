#include "VirtualCamSender.h"
#include "shared.inl"
#include <limits>

VirtualCamSender::VirtualCamSender()
    : m_deviceIndex(0)
{
}

VirtualCamSender::~VirtualCamSender()
{
    Shutdown();
}

bool VirtualCamSender::Initialize(int deviceIndex)
{
    Shutdown();
    m_deviceIndex = deviceIndex;
    m_shm = std::make_unique<SharedImageMemory>(deviceIndex);
    return (m_shm != nullptr);
}

void VirtualCamSender::Shutdown()
{
    if (m_shm)
    {
        m_shm.reset();
    }
}

bool VirtualCamSender::IsConnected()
{
    if (!m_shm) return false;
    return m_shm->SendIsReady();
}

bool VirtualCamSender::SendFrame(int width, int height, const uint8_t* rgbaBuffer)
{
    if (!m_shm || !rgbaBuffer) return false;

    // If Chrome or a receiving app hasn't opened the virtual camera yet, SendIsReady returns false
    if (!m_shm->SendIsReady())
    {
        return false;
    }

    int stride = width * 4;
    DWORD dataSize = width * height * 4;
    auto format = SharedImageMemory::FORMAT_UINT8;
    auto resizeMode = SharedImageMemory::RESIZEMODE_LINEAR;
    auto mirrorMode = SharedImageMemory::MIRRORMODE_DISABLED;
    int timeout = (std::numeric_limits<int>::max)() - SharedImageMemory::RECEIVE_MAX_WAIT;

    SharedImageMemory::ESendResult res = m_shm->Send(
        width,
        height,
        stride,
        dataSize,
        format,
        resizeMode,
        mirrorMode,
        timeout,
        rgbaBuffer
    );

    return (res == SharedImageMemory::SENDRES_OK || res == SharedImageMemory::SENDRES_WARN_FRAMESKIP);
}

bool VirtualCamSender::SendFrameBGRA(int width, int height, const uint8_t* bgraBuffer)
{
    if (!m_shm || !bgraBuffer) return false;

    // Fast check if receiving application (Chrome) is capturing
    if (!m_shm->SendIsReady())
    {
        return false;
    }

    size_t totalBytes = (size_t)width * height * 4;
    if (m_conversionBuffer.size() != totalBytes)
    {
        m_conversionBuffer.resize(totalBytes);
    }

    // Fast SIMD/32-bit swap of Blue and Red channels
    // In little-endian:
    // BGRA pixel is 0xAARRGGBB -> convert to RGBA 0xAABBGGRR
    const uint32_t* src32 = reinterpret_cast<const uint32_t*>(bgraBuffer);
    uint32_t* dst32 = reinterpret_cast<uint32_t*>(m_conversionBuffer.data());
    int pixelCount = width * height;

    for (int i = 0; i < pixelCount; ++i)
    {
        uint32_t px = src32[i];
        dst32[i] = (px & 0xFF00FF00) | ((px & 0x00FF0000) >> 16) | ((px & 0x000000FF) << 16);
    }

    return SendFrame(width, height, m_conversionBuffer.data());
}

