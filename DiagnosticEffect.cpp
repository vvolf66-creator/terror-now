#include "DiagnosticEffect.h"
#include <cstring>

DiagnosticEffect::DiagnosticEffect()
{
}

void DiagnosticEffect::Apply(const uint8_t* srcRgba, uint8_t* dstRgba, int width, int height)
{
    if (!srcRgba || !dstRgba) return;

    size_t totalBytes = (size_t)width * height * 4;
    memcpy(dstRgba, srcRgba, totalBytes);

    int channelShift = 14 * 4; // Shift red channel by 14 pixels

    for (int y = 0; y < height; y++)
    {
        bool isScanline = ((y % 16) < 4);
        int rowOffset = y * width * 4;

        for (int x = 0; x < width * 4; x += 4)
        {
            int idx = rowOffset + x;

            uint8_t r = srcRgba[idx + 0];
            uint8_t g = srcRgba[idx + 1];
            uint8_t b = srcRgba[idx + 2];
            uint8_t a = srcRgba[idx + 3];

            if (isScanline)
            {
                // Invert colors on diagnostic scanlines
                r = 255 - r;
                g = 255 - g;
                b = 255 - b;
            }

            // Chromatic displacement on red channel
            if (idx + channelShift + 0 < (int)totalBytes)
            {
                r = srcRgba[idx + channelShift + 0];
            }

            dstRgba[idx + 0] = r;
            dstRgba[idx + 1] = g;
            dstRgba[idx + 2] = b;
            dstRgba[idx + 3] = a;
        }
    }
}
