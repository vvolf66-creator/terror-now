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
