#pragma once

#include <cstdint>
#include <vector>

class DiagnosticEffect
{
public:
    DiagnosticEffect();

    // Inverts every few scanlines and shifts the red channel horizontally
    // to provide visual proof of frame switching
    void Apply(const uint8_t* srcRgba, uint8_t* dstRgba, int width, int height);
};
