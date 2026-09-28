#include "CameraCapture.h"
#include <mferror.h>
#include <iostream>

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

    IMFAttributes* pAttributes = nullptr;
    IMFActivate** ppDevices = nullptr;
    UINT32 count = 0;
    IMFMediaSource* pSource = nullptr;

    HRESULT hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) return false;

    hr = pAttributes->SetGUID(
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
        MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
    );
    if (FAILED(hr)) { SafeRelease(&pAttributes); return false; }

    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &count);
    SafeRelease(&pAttributes);
    if (FAILED(hr) || count == 0) return false;

    if (deviceIndex < 0 || deviceIndex >= (int)count) deviceIndex = 0;

    // Get friendly name of the physical webcam
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

    if (FAILED(hr)) return false;

    // Create SourceReader with low latency and hardware acceleration attributes
    hr = MFCreateAttributes(&pAttributes, 2);
    if (SUCCEEDED(hr))
    {
        pAttributes->SetUINT32(MF_READWRITE_ENABLE_HARDWARE_TRANSFORMS, TRUE);
        pAttributes->SetUINT32(MF_LOW_LATENCY, TRUE);
    }

    hr = MFCreateSourceReaderFromMediaSource(pSource, pAttributes, &m_pReader);
    SafeRelease(&pAttributes);
    SafeRelease(&pSource);

    if (FAILED(hr)) return false;

    // Configure Media Type (Resolution & RGB32 output)
    if (!ConfigureMediaType(targetWidth, targetHeight, targetFps))
    {
        // Retry a smaller format, and fail if neither format was negotiated.
        if (targetWidth == 640 || !ConfigureMediaType(640, 480, 30))
        {
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

    // Set output format on SourceReader to uncompressed RGB32
    // Windows Media Foundation automatically inserts internal color converter transforms if needed
    IMFMediaType* pMediaType = nullptr;
    HRESULT hr = MFCreateMediaType(&pMediaType);
    if (FAILED(hr)) return false;

    hr = pMediaType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    if (SUCCEEDED(hr)) hr = pMediaType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (SUCCEEDED(hr)) hr = MFSetAttributeSize(pMediaType, MF_MT_FRAME_SIZE, targetWidth, targetHeight);
    if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(pMediaType, MF_MT_FRAME_RATE, targetFps, 1);

    if (SUCCEEDED(hr)) hr = m_pReader->SetCurrentMediaType((DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, NULL, pMediaType);
    SafeRelease(&pMediaType);

    if (SUCCEEDED(hr))
    {
        m_actualWidth = targetWidth;
        m_actualHeight = targetHeight;
        m_videoSubtype = MFVideoFormat_RGB32;
        return true;
    }

    // Do not reinterpret an arbitrary native media type as tightly packed RGB32.
    return false;
}

void CameraCapture::Stop()
{
    m_isCapturing = false;
    SafeRelease(&m_pReader);
    m_actualWidth = 0;
    m_actualHeight = 0;
}

bool CameraCapture::ReadFrame(std::vector<uint8_t>& outRgbaBuffer, int& outWidth, int& outHeight)
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

    if (FAILED(hr) || !pSample)
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
        if (outRgbaBuffer.size() != requiredSize)
        {
            outRgbaBuffer.resize(requiredSize);
        }

        if (m_videoSubtype == MFVideoFormat_RGB32)
        {
            // Media Foundation RGB32 is B,G,R,X. Unity Capture UINT8 expects R,G,B,A.
            if (cbLen < requiredSize)
            {
                pBuffer->Unlock();
                SafeRelease(&pBuffer);
                SafeRelease(&pSample);
                return false;
            }
            for (size_t i = 0; i < requiredSize; i += 4)
            {
                outRgbaBuffer[i] = pData[i + 2];
                outRgbaBuffer[i + 1] = pData[i + 1];
                outRgbaBuffer[i + 2] = pData[i];
                outRgbaBuffer[i + 3] = 255;
            }
        }
        else if (m_videoSubtype == MFVideoFormat_YUY2)
        {
            ConvertYUY2ToRGBA(pData, outRgbaBuffer.data(), outWidth, outHeight);
        }
        else if (m_videoSubtype == MFVideoFormat_NV12)
        {
            ConvertNV12ToRGBA(pData, outRgbaBuffer.data(), outWidth, outHeight);
        }
        else
        {
            // Fallback direct copy
            size_t copyBytes = (cbLen < requiredSize) ? cbLen : requiredSize;
            memcpy(outRgbaBuffer.data(), pData, copyBytes);
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

void CameraCapture::ConvertYUY2ToRGBA(const uint8_t* yuy2, uint8_t* rgba, int width, int height)
{
    int pixelCount = width * height;
    for (int i = 0, j = 0; i < pixelCount; i += 2, j += 4)
    {
        int y0 = yuy2[j + 0];
        int u0 = yuy2[j + 1] - 128;
        int y1 = yuy2[j + 2];
        int v0 = yuy2[j + 3] - 128;

        int r0 = y0 + ((1402 * v0) >> 10);
        int g0 = y0 - ((344 * u0 + 714 * v0) >> 10);
        int b0 = y0 + ((1772 * u0) >> 10);

        int r1 = y1 + ((1402 * v0) >> 10);
        int g1 = y1 - ((344 * u0 + 714 * v0) >> 10);
        int b1 = y1 + ((1772 * u0) >> 10);

        int outIdx0 = i * 4;
        rgba[outIdx0 + 0] = ClampByte(r0);
        rgba[outIdx0 + 1] = ClampByte(g0);
        rgba[outIdx0 + 2] = ClampByte(b0);
        rgba[outIdx0 + 3] = 255;

        int outIdx1 = (i + 1) * 4;
        rgba[outIdx1 + 0] = ClampByte(r1);
        rgba[outIdx1 + 1] = ClampByte(g1);
        rgba[outIdx1 + 2] = ClampByte(b1);
        rgba[outIdx1 + 3] = 255;
    }
}

void CameraCapture::ConvertNV12ToRGBA(const uint8_t* nv12, uint8_t* rgba, int width, int height)
{
    const uint8_t* yPlane = nv12;
    const uint8_t* uvPlane = nv12 + (width * height);

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            int yVal = yPlane[y * width + x];
            int uvIdx = (y / 2) * width + (x & ~1);
            int uVal = uvPlane[uvIdx] - 128;
            int vVal = uvPlane[uvIdx + 1] - 128;

            int r = yVal + ((1402 * vVal) >> 10);
            int g = yVal - ((344 * uVal + 714 * vVal) >> 10);
            int b = yVal + ((1772 * uVal) >> 10);

            int outIdx = (y * width + x) * 4;
            rgba[outIdx + 0] = ClampByte(r);
            rgba[outIdx + 1] = ClampByte(g);
            rgba[outIdx + 2] = ClampByte(b);
            rgba[outIdx + 3] = 255;
        }
    }
}
