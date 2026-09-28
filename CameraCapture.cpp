#include "CameraCapture.h"
#include <mferror.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "ole32.lib")

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

CameraCapture::CameraCapture()
    : m_pReader(nullptr)
    , m_isCapturing(false)
    , m_actualWidth(0)
    , m_actualHeight(0)
    , m_stride(0)
    , m_isBottomUp(false)
    , m_videoSubtype(MFVideoFormat_RGB32)
{
}

CameraCapture::~CameraCapture()
{
    Stop();
}

bool CameraCapture::InitializeMF()
{
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE)
    {
        return false;
    }
    hr = MFStartup(MF_VERSION);
    return SUCCEEDED(hr);
}

void CameraCapture::ShutdownMF()
{
    MFShutdown();
    CoUninitialize();
}

bool CameraCapture::Start(int deviceIndex, int targetWidth, int targetHeight, int targetFps)
{
    Stop();
    m_lastError.clear();

    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    IMFMediaSource* pSource = nullptr;

    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr))
    {
        m_lastError = "Failed to allocate Media Foundation attributes.";
        return false;
    }

    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr))
    {
        SafeRelease(&pAttributes);
        m_lastError = "Failed to set video capture attribute.";
        return false;
    }

    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    SafeRelease(&pAttributes);
    if (FAILED(hr) || count == 0)
    {
        m_lastError = "No physical webcams detected on this system.";
        return false;
    }

    if (deviceIndex >= (int)count) deviceIndex = 0;

    // Get friendly device name
    WCHAR name[256] = { 0 };
    UINT32 nameLen = 0;
    ppDevices[deviceIndex]->GetString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, name, 256, &nameLen);
    m_deviceName = name;

    // Activate hardware device
    hr = ppDevices[deviceIndex]->ActivateObject(IID_PPV_ARGS(&pSource));
    for (UINT32 i = 0; i < count; i++)
    {
        SafeRelease(&ppDevices[i]);
    }
    CoTaskMemFree(ppDevices);

    if (FAILED(hr))
    {
        std::stringstream ss;
        ss << "Camera device busy or access denied (hr=0x" << std::hex << hr << "). Check Windows Camera Privacy settings.";
        m_lastError = ss.str();
        return false;
    }

    // Configure SourceReader attributes for low latency and hardware acceleration
    hr = MFCreateAttributes(&pAttributes, 2);
    if (SUCCEEDED(hr))
    {
        pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    }

    hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &m_pReader);
    SafeRelease(&pAttributes);
    SafeRelease(&pSource);

    if (FAILED(hr))
    {
        m_lastError = "Failed to create Media Foundation SourceReader.";
        return false;
    }

    // Configure Media Type (Target resolution & uncompressed format)
    if (!ConfigureMediaType(targetWidth, targetHeight, targetFps))
    {
        // Try fallback to 640x480 if target was 720p
        if (targetWidth != 640 && !ConfigureMediaType(640, 480, 30))
        {
            m_lastError = "Could not negotiate compatible video format (720p or 480p).";
            Stop();
            return false;
        }
    }

    m_isCapturing = true;
    return true;
}

bool CameraCapture::ConfigureMediaType(int targetWidth, int targetHeight, int targetFps)
{
    if (!m_pReader) return false;

    IMFMediaType* pMediaType = nullptr;
    HRESULT hr = MFCreateMediaType(&pMediaType);
    if (FAILED(hr)) return false;

    hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, targetWidth, targetHeight);
    hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, targetFps, 1);

    hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pMediaType);
    SafeRelease(&pMediaType);

    IMFMediaType* pCurrentType = nullptr;
    if (SUCCEEDED(hr))
    {
        hr = m_pReader->GetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, &pCurrentType);
    }
    // Never interpret a native YUY2/NV12/MJPEG sample as RGB32 when conversion negotiation failed.

    if (SUCCEEDED(hr) && pCurrentType)
    {
        UINT32 w = 0, h = 0;
        MFGetAttributeSize(pCurrentType, MF_MT_FRAME_SIZE, &w, &h);
        pCurrentType->GetGUID(MF_MT_SUBTYPE, &m_videoSubtype);
        if (m_videoSubtype != MFVideoFormat_RGB32 || !w || !h)
        {
            SafeRelease(&pCurrentType);
            return false;
        }
        m_actualWidth = (int)w;
        m_actualHeight = (int)h;

        // Query default stride to properly determine row pitch and bottom-up/top-down orientation
        UINT32 strideVal = 0;
        HRESULT hrStride = pCurrentType->GetUINT32(MF_MT_DEFAULT_STRIDE, &strideVal);
        if (SUCCEEDED(hrStride))
        {
            INT32 s = (INT32)strideVal;
            if (s < 0)
            {
                m_isBottomUp = true;
                m_stride = -s;
            }
            else
            {
                m_isBottomUp = false;
                m_stride = s;
            }
        }
        else
        {
            // Fallback default stride
            m_isBottomUp = false;
            m_stride = m_actualWidth * 4;
        }

        SafeRelease(&pCurrentType);
        return true;
    }

    return false;
}

void CameraCapture::Stop()
{
    m_isCapturing = false;
    SafeRelease(&m_pReader);
    m_actualWidth = 0;
    m_actualHeight = 0;
    m_stride = 0;
    m_isBottomUp = false;
}

bool CameraCapture::ReadFrame(std::vector<uint8_t>& outBgraBuffer, int& outWidth, int& outHeight)
{
    if (!m_isCapturing || !m_pReader) return false;

    DWORD streamIndex = 0;
    DWORD flags = 0;
    LONGLONG timestamp = 0;
    IMFSample* pSample = nullptr;

    HRESULT hr = m_pReader->ReadSample(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
        0,
        &streamIndex,
        &flags,
        &timestamp,
        &pSample
    );

    if (FAILED(hr) || (flags & (MF_SOURCE_READERF_ERROR | MF_SOURCE_READERF_ENDOFSTREAM)) || !pSample)
    {
        return false;
    }

    IMFMediaBuffer* pBuffer = nullptr;
    hr = pSample->ConvertToContiguousBuffer(&pBuffer);
    if (FAILED(hr))
    {
        SafeRelease(&pSample);
        return false;
    }

    BYTE* pData = nullptr;
    DWORD cbLen = 0;
    hr = pBuffer->Lock(&pData, NULL, &cbLen);

    if (SUCCEEDED(hr) && pData)
    {
        outWidth = m_actualWidth;
        outHeight = m_actualHeight;
        size_t requiredSize = (size_t)outWidth * outHeight * 4;
        if (outBgraBuffer.size() != requiredSize)
        {
            outBgraBuffer.resize(requiredSize);
        }

        int rowBytes = outWidth * 4;
        int effectiveStride = (m_stride > 0) ? m_stride : rowBytes;

        if (effectiveStride < rowBytes || cbLen < (size_t)(outHeight - 1) * effectiveStride + rowBytes)
        {
            pBuffer->Unlock();
            SafeRelease(&pBuffer);
            SafeRelease(&pSample);
            return false;
        }

        if (m_videoSubtype == MFVideoFormat_RGB32)
        {
            // Unpack row-by-row respecting stride and bottom-up/top-down orientation
            for (int y = 0; y < outHeight; ++y)
            {
                int srcY = m_isBottomUp ? (outHeight - 1 - y) : y;
                const uint8_t* pSrcRow = pData + (srcY * effectiveStride);
                uint8_t* pDstRow = outBgraBuffer.data() + (y * rowBytes);

                memcpy(pDstRow, pSrcRow, rowBytes);

                // Ensure alpha channel is solid 255
                for (int x = 0; x < outWidth; ++x)
                {
                    pDstRow[x * 4 + 3] = 255;
                }
            }
        }
        else if (m_videoSubtype == MFVideoFormat_YUY2)
        {
            ConvertYUY2ToBGRA(pData, outBgraBuffer.data(), outWidth, outHeight, effectiveStride, m_isBottomUp);
        }
        else if (m_videoSubtype == MFVideoFormat_NV12)
        {
            ConvertNV12ToBGRA(pData, outBgraBuffer.data(), outWidth, outHeight, effectiveStride, m_isBottomUp);
        }
        else
        {
            // General copy
            size_t copyBytes = (cbLen < requiredSize) ? cbLen : requiredSize;
            memcpy(outBgraBuffer.data(), pData, copyBytes);
        }

        pBuffer->Unlock();
        SafeRelease(&pBuffer);
        SafeRelease(&pSample);
        return true;
    }

    SafeRelease(&pBuffer);
    SafeRelease(&pSample);
    return false;
}

static inline uint8_t ClampByte(int val)
{
    return (uint8_t)(val < 0 ? 0 : (val > 255 ? 255 : val));
}

// Converts YUY2 to clean top-down 32-bit BGRA (B=0, G=1, R=2, A=3)
void CameraCapture::ConvertYUY2ToBGRA(const uint8_t* yuy2, uint8_t* bgra, int width, int height, int stride, bool isBottomUp)
{
    for (int y = 0; y < height; ++y)
    {
        int srcY = isBottomUp ? (height - 1 - y) : y;
        const uint8_t* pSrcRow = yuy2 + (srcY * (width * 2));
        uint8_t* pDstRow = bgra + (y * width * 4);

        for (int x = 0, j = 0; x < width; x += 2, j += 4)
        {
            int y0 = pSrcRow[j + 0];
            int u0 = pSrcRow[j + 1] - 128;
            int y1 = pSrcRow[j + 2];
            int v0 = pSrcRow[j + 3] - 128;

            int r0 = y0 + ((1402 * v0) >> 10);
            int g0 = y0 - ((344 * u0 + 714 * v0) >> 10);
            int b0 = y0 + ((1772 * u0) >> 10);

            int r1 = y1 + ((1402 * v0) >> 10);
            int g1 = y1 - ((344 * u0 + 714 * v0) >> 10);
            int b1 = y1 + ((1772 * u0) >> 10);

            // Pixel 0 (BGRA)
            pDstRow[x * 4 + 0] = ClampByte(b0);
            pDstRow[x * 4 + 1] = ClampByte(g0);
            pDstRow[x * 4 + 2] = ClampByte(r0);
            pDstRow[x * 4 + 3] = 255;

            // Pixel 1 (BGRA)
            pDstRow[(x + 1) * 4 + 0] = ClampByte(b1);
            pDstRow[(x + 1) * 4 + 1] = ClampByte(g1);
            pDstRow[(x + 1) * 4 + 2] = ClampByte(r1);
            pDstRow[(x + 1) * 4 + 3] = 255;
        }
    }
}

// Converts NV12 to clean top-down 32-bit BGRA (B=0, G=1, R=2, A=3)
void CameraCapture::ConvertNV12ToBGRA(const uint8_t* nv12, uint8_t* bgra, int width, int height, int stride, bool isBottomUp)
{
    const uint8_t* yPlane = nv12;
    const uint8_t* uvPlane = nv12 + (width * height);

    for (int y = 0; y < height; ++y)
    {
        int srcY = isBottomUp ? (height - 1 - y) : y;
        const uint8_t* pYRow = yPlane + (srcY * width);
        const uint8_t* pUVRow = uvPlane + ((srcY / 2) * width);
        uint8_t* pDstRow = bgra + (y * width * 4);

        for (int x = 0; x < width; ++x)
        {
            int yVal = pYRow[x];
            int uvIdx = (x & ~1);
            int uVal = pUVRow[uvIdx + 0] - 128;
            int vVal = pUVRow[uvIdx + 1] - 128;

            int r = yVal + ((1402 * vVal) >> 10);
            int g = yVal - ((344 * uVal + 714 * vVal) >> 10);
            int b = yVal + ((1772 * uVal) >> 10);

            // BGRA layout
            pDstRow[x * 4 + 0] = ClampByte(b);
            pDstRow[x * 4 + 1] = ClampByte(g);
            pDstRow[x * 4 + 2] = ClampByte(r);
            pDstRow[x * 4 + 3] = 255;
        }
    }
}
