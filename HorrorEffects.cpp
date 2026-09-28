#include "HorrorEffects.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <random>

static inline uint8_t ClampByte(int val)
{
    return (uint8_t)(val < 0 ? 0 : (val > 255 ? 255 : val));
}

// Fast pseudo-random number generator for screen shake and static
static uint32_t FastRand(uint32_t& state)
{
    state = state * 1664525u + 1013904223u;
    return state;
}

HorrorEffects::HorrorEffects()
    : m_currentEffect(HorrorEffectType::NONE)
    , m_durationSeconds(0.0f)
    , m_historyWidth(0)
    , m_historyHeight(0)
    , m_maskWidth(320)
    , m_maskHeight(320)
{
    GenerateCreatureFace(m_maskWidth, m_maskHeight);
}

HorrorEffects::~HorrorEffects()
{
}

void HorrorEffects::TriggerEffect(HorrorEffectType type)
{
    m_currentEffect = type;
    m_startTime = std::chrono::steady_clock::now();

    switch (type)
    {
    case HorrorEffectType::EDGE_LURKER:
        m_durationSeconds = 2.4f; // 2.4 seconds total
        break;
    case HorrorEffectType::SUDDEN_LUNGER:
        m_durationSeconds = 1.05f; // Fast, violent 1-second jumpscare
        break;
    case HorrorEffectType::HAUNTED_ANOMALY:
        m_durationSeconds = 2.4f; // 2.4 seconds total
        break;
    default:
        m_durationSeconds = 0.0f;
        break;
    }
}

void HorrorEffects::RestoreNormal()
{
    m_currentEffect = HorrorEffectType::NONE;
}

bool HorrorEffects::IsActive() const
{
    if (m_currentEffect == HorrorEffectType::NONE) return false;
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();
    return (elapsed < m_durationSeconds);
}

std::string HorrorEffects::GetCurrentEffectName() const
{
    switch (m_currentEffect)
    {
    case HorrorEffectType::EDGE_LURKER:
        return "EFFECT 1: THE EDGE LURKER";
    case HorrorEffectType::SUDDEN_LUNGER:
        return "EFFECT 2: SUDDEN LUNGER (JUMPSCARE)";
    case HorrorEffectType::HAUNTED_ANOMALY:
        return "EFFECT 3: HAUNTED ROOM ANOMALY";
    default:
        return "NORMAL (PASSTHROUGH)";
    }
}

float HorrorEffects::GetProgress() const
{
    if (!IsActive()) return 0.0f;
    auto now = std::chrono::steady_clock::now();
    float elapsed = std::chrono::duration<float>(now - m_startTime).count();
    float p = elapsed / m_durationSeconds;
    return (p > 1.0f) ? 1.0f : (p < 0.0f ? 0.0f : p);
}

// Procedural generation of a high-detail creature face mask (BGRA)
void HorrorEffects::GenerateCreatureFace(int w, int h)
{
    m_creatureFaceMask.resize((size_t)w * h * 4);
    float cx = w * 0.5f;
    float cy = h * 0.48f;

    for (int y = 0; y < h; ++y)
    {
        for (int x = 0; x < w; ++x)
        {
            float dx = (x - cx) / (w * 0.42f);
            float dy = (y - cy) / (h * 0.48f);
            float dist = std::sqrt(dx * dx + dy * dy);

            int idx = (y * w + x) * 4;

            if (dist > 1.0f)
            {
                // Fully transparent outside outer bounds
                m_creatureFaceMask[idx + 0] = 0;
                m_creatureFaceMask[idx + 1] = 0;
                m_creatureFaceMask[idx + 2] = 0;
                m_creatureFaceMask[idx + 3] = 0;
                continue;
            }

            // Base skin tone: Pale, ashen, sickly necrotic grey with cold blue-green veins
            float baseAlpha = std::pow(1.0f - dist, 0.45f);
            uint8_t alpha = ClampByte((int)(baseAlpha * 255.0f));

            uint8_t b = 185;
            uint8_t g = 195;
            uint8_t r = 180;

            // Eye sockets (Sunken deep black caverns)
            float eyeY = cy - h * 0.10f;
            float leftEyeX = cx - w * 0.18f;
            float rightEyeX = cx + w * 0.18f;

            float dLeftEye = std::sqrt(std::pow((x - leftEyeX) / (w * 0.11f), 2.0f) + std::pow((y - eyeY) / (h * 0.13f), 2.0f));
            float dRightEye = std::sqrt(std::pow((x - rightEyeX) / (w * 0.11f), 2.0f) + std::pow((y - eyeY) / (h * 0.13f), 2.0f));

            if (dLeftEye < 1.0f || dRightEye < 1.0f)
            {
                float eyeDist = (std::min)(dLeftEye, dRightEye);
                float shadow = std::pow(eyeDist, 0.6f);
                b = (uint8_t)(b * shadow * 0.15f);
                g = (uint8_t)(g * shadow * 0.15f);
                r = (uint8_t)(r * shadow * 0.15f);

                // Tiny white glowing reflective eye pinpricks in center
                if (eyeDist < 0.18f)
                {
                    b = 255; g = 255; r = 255;
                }
            }

            // Mouth: Gaping, pitch-black screaming maw with needle-like teeth
            float mouthX = cx;
            float mouthY = cy + h * 0.22f;
            float dMouth = std::sqrt(std::pow((x - mouthX) / (w * 0.22f), 2.0f) + std::pow((y - mouthY) / (h * 0.26f), 2.0f));
            if (dMouth < 1.0f)
            {
                float mouthDarkness = std::pow(dMouth, 0.4f);
                b = (uint8_t)(15 * mouthDarkness);
                g = (uint8_t)(8 * mouthDarkness);
                r = (uint8_t)(25 * mouthDarkness);

                // Jagged white teeth on upper and lower perimeter
                float teethBand = std::abs(dMouth - 0.75f);
                if (teethBand < 0.12f && (x % 11 < 5))
                {
                    b = 220; g = 230; r = 240;
                }
            }

            // Facial shading: cheekbone hollows and nasal cavity
            float dNose = std::sqrt(std::pow((x - cx) / (w * 0.06f), 2.0f) + std::pow((y - (cy + h * 0.04f)) / (h * 0.07f), 2.0f));
            if (dNose < 1.0f)
            {
                b = (uint8_t)(b * 0.3f);
                g = (uint8_t)(g * 0.2f);
                r = (uint8_t)(r * 0.3f);
            }

            m_creatureFaceMask[idx + 0] = b;
            m_creatureFaceMask[idx + 1] = g;
            m_creatureFaceMask[idx + 2] = r;
            m_creatureFaceMask[idx + 3] = alpha;
        }
    }
}

void HorrorEffects::ProcessFrame(const uint8_t* srcBgra, uint8_t* dstBgra, int width, int height)
{
    if (!IsActive())
    {
        // Unmodified clean passthrough
        m_currentEffect = HorrorEffectType::NONE;
        memcpy(dstBgra, srcBgra, (size_t)width * height * 4);
        return;
    }

    float progress = GetProgress();

    switch (m_currentEffect)
    {
    case HorrorEffectType::EDGE_LURKER:
        RenderEdgeLurker(srcBgra, dstBgra, width, height, progress);
        break;

    case HorrorEffectType::SUDDEN_LUNGER:
        RenderSuddenLunger(srcBgra, dstBgra, width, height, progress);
        break;

    case HorrorEffectType::HAUNTED_ANOMALY:
        RenderHauntedAnomaly(srcBgra, dstBgra, width, height, progress);
        break;

    default:
        memcpy(dstBgra, srcBgra, (size_t)width * height * 4);
        break;
    }
}

// ============================================================================
// EFFECT 1: THE EDGE LURKER
// A dark, disturbing silhouette slowly emerges from the screen edge, peeks in
// with reflective pale eyes, lingers for reaction, then slinks back out.
// ============================================================================
void HorrorEffects::RenderEdgeLurker(const uint8_t* src, uint8_t* dst, int w, int h, float progress)
{
    // Progress breakdown:
    // 0.0 - 0.20: Edge shadow dims
    // 0.20 - 0.50: Figure slides into frame (from left edge)
    // 0.50 - 0.75: Static tilted head gaze with subtle breathing
    // 0.75 - 1.00: Smoothly slides back behind edge, lighting normalizes

    float figureXOffset = 0.0f;
    float edgeDarkening = 0.0f;
    float eyeIntensity = 0.0f;

    if (progress < 0.20f)
    {
        float p = progress / 0.20f;
        edgeDarkening = p * 0.45f;
        figureXOffset = -0.15f * w;
        eyeIntensity = 0.0f;
    }
    else if (progress < 0.50f)
    {
        float p = (progress - 0.20f) / 0.30f;
        float ease = std::sin(p * 3.14159f * 0.5f); // Smooth enter
        figureXOffset = -0.15f * w + ease * (0.24f * w);
        edgeDarkening = 0.45f + ease * 0.25f;
        eyeIntensity = ease;
    }
    else if (progress < 0.75f)
    {
        // Lingering gaze with subtle breathing
        float p = (progress - 0.50f) / 0.25f;
        float breathe = std::sin(p * 6.28318f) * 3.0f;
        figureXOffset = 0.09f * w + breathe;
        edgeDarkening = 0.70f;
        eyeIntensity = 1.0f;
    }
    else
    {
        float p = (progress - 0.75f) / 0.25f;
        float ease = 1.0f - std::sin(p * 3.14159f * 0.5f); // Smooth exit
        figureXOffset = -0.15f * w + ease * (0.24f * w);
        edgeDarkening = ease * 0.70f;
        eyeIntensity = ease;
    }

    // Lurker anatomical coordinates
    float headCenterX = figureXOffset;
    float headCenterY = h * 0.42f;
    float headRadiusX = w * 0.085f;
    float headRadiusY = h * 0.16f;

    float eyeY = headCenterY - h * 0.02f;
    float leftEyeX = headCenterX - w * 0.022f;
    float rightEyeX = headCenterX + w * 0.022f;

    for (int y = 0; y < h; ++y)
    {
        // Left peripheral darkening vignette
        float edgeFactor = 1.0f - (std::min)(1.0f, (float)y / (h * 0.3f)); // blend
        float xVignette = (std::max)(0.0f, 1.0f - ((float)y / h));
        float leftShadow = (std::max)(0.0f, 1.0f - ((float)y / (w * 0.4f)));

        int rowOffset = y * w * 4;

        for (int x = 0; x < w; ++x)
        {
            int idx = rowOffset + x * 4;
            uint8_t srcB = src[idx + 0];
            uint8_t srcG = src[idx + 1];
            uint8_t srcR = src[idx + 2];

            // 1. Apply peripheral shadow near left edge
            float xDistFromLeft = (float)x / (w * 0.35f);
            if (xDistFromLeft < 1.0f)
            {
                float shadowAmount = (1.0f - xDistFromLeft) * edgeDarkening;
                srcB = (uint8_t)(srcB * (1.0f - shadowAmount * 0.65f));
                srcG = (uint8_t)(srcG * (1.0f - shadowAmount * 0.70f));
                srcR = (uint8_t)(srcR * (1.0f - shadowAmount * 0.75f));
            }

            // 2. Head and body silhouette of the Lurker
            float dxHead = (x - headCenterX) / headRadiusX;
            float dyHead = (y - headCenterY) / headRadiusY;
            float dHead = dxHead * dxHead + dyHead * dyHead;

            // Elongated body/neck sloping down
            float neckY = headCenterY + headRadiusY * 0.7f;
            bool isBody = (y > neckY) && (x < headCenterX + headRadiusX * 1.3f + (y - neckY) * 0.4f);

            float silhouetteAlpha = 0.0f;
            if (dHead < 1.0f)
            {
                silhouetteAlpha = (std::min)(1.0f, (1.0f - dHead) * 3.5f);
            }
            else if (isBody)
            {
                float bodyDist = (headCenterX + headRadiusX * 1.3f + (y - neckY) * 0.4f) - x;
                silhouetteAlpha = (std::min)(1.0f, bodyDist / 20.0f);
            }

            if (silhouetteAlpha > 0.0f)
            {
                // Pitch black shadowy entity with cold blue tint
                uint8_t entB = 16;
                uint8_t entG = 10;
                uint8_t entR = 12;

                // Check for reflective pale sunken eyes
                float dEyeL = std::sqrt(std::pow((x - leftEyeX) / 4.0f, 2.0f) + std::pow((y - eyeY) / 5.5f, 2.0f));
                float dEyeR = std::sqrt(std::pow((x - rightEyeX) / 4.0f, 2.0f) + std::pow((y - eyeY) / 5.5f, 2.0f));
                float minEye = (std::min)(dEyeL, dEyeR);

                if (minEye < 1.0f && eyeIntensity > 0.1f)
                {
                    float eyeGlow = (1.0f - minEye) * eyeIntensity;
                    entB = (uint8_t)(std::min)(255, (int)(entB + eyeGlow * 240));
                    entG = (uint8_t)(std::min)(255, (int)(entG + eyeGlow * 245));
                    entR = (uint8_t)(std::min)(255, (int)(entR + eyeGlow * 225));
                }

                srcB = (uint8_t)(srcB * (1.0f - silhouetteAlpha) + entB * silhouetteAlpha);
                srcG = (uint8_t)(srcG * (1.0f - silhouetteAlpha) + entG * silhouetteAlpha);
                srcR = (uint8_t)(srcR * (1.0f - silhouetteAlpha) + entR * silhouetteAlpha);
            }

            dst[idx + 0] = srcB;
            dst[idx + 1] = srcG;
            dst[idx + 2] = srcR;
            dst[idx + 3] = 255;
        }
    }
}

// ============================================================================
// EFFECT 2: THE SUDDEN LUNGER (JUMPSCARE)
// A terrifying creature violently rushes from the darkness into the camera lens
// with violent screen vibration, radial motion blur, and chromatic tearing.
// ============================================================================
void HorrorEffects::RenderSuddenLunger(const uint8_t* src, uint8_t* dst, int w, int h, float progress)
{
    // Progress breakdown (1.05s total):
    // 0.0 - 0.10: Single-frame blackout & horizontal chromatic glitch (anticipation pop)
    // 0.10 - 0.70: Violent forward lunge from scale 0.18x to 1.85x right into screen
    // 0.70 - 1.00: Heavy static sync tear and instant snap back to clean normal feed

    uint32_t rng = (uint32_t)(progress * 100000.0f) + 12345u;

    if (progress < 0.10f)
    {
        // Anticipation glitch: brief dark pop with severe chromatic horizontal displacement
        int shift = 16;
        for (int y = 0; y < h; ++y)
        {
            int rowOffset = y * w * 4;
            bool isScanline = (y % 4 == 0);
            for (int x = 0; x < w; ++x)
            {
                int idx = rowOffset + x * 4;
                int rX = (std::min)(w - 1, x + shift);
                int bX = (std::max)(0, x - shift);

                uint8_t b = src[rowOffset + bX * 4 + 0];
                uint8_t g = src[rowOffset + x * 4 + 1];
                uint8_t r = src[rowOffset + rX * 4 + 2];

                // Contrast crush
                b = (uint8_t)(b * 0.4f);
                g = (uint8_t)(g * 0.4f);
                r = (uint8_t)(r * 0.6f);

                if (isScanline)
                {
                    b = 255 - b;
                    g = 255 - g;
                    r = 255 - r;
                }

                dst[idx + 0] = b;
                dst[idx + 1] = g;
                dst[idx + 2] = r;
                dst[idx + 3] = 255;
            }
        }
        return;
    }

    if (progress >= 0.70f)
    {
        // Static tear dissolving creature back to clean feed
        float tearFactor = 1.0f - ((progress - 0.70f) / 0.30f);
        for (int y = 0; y < h; ++y)
        {
            int rowOffset = y * w * 4;
            int xOffset = (static_cast<int>(FastRand(rng) % 21) - 10) * (int)(tearFactor * 2.0f);
            for (int x = 0; x < w; ++x)
            {
                int idx = rowOffset + x * 4;
                int sampleX = std::clamp(x + xOffset, 0, w - 1);
                int srcIdx = rowOffset + sampleX * 4;

                uint8_t b = src[srcIdx + 0];
                uint8_t g = src[srcIdx + 1];
                uint8_t r = src[srcIdx + 2];

                if (FastRand(rng) % 100 < (uint32_t)(tearFactor * 35.0f))
                {
                    uint8_t noise = (uint8_t)(FastRand(rng) % 256);
                    b = noise; g = noise; r = noise;
                }

                dst[idx + 0] = b;
                dst[idx + 1] = g;
                dst[idx + 2] = r;
                dst[idx + 3] = 255;
            }
        }
        return;
    }

    // MAIN LUNGE PHASE: 0.10 to 0.70
    float lungeNorm = (progress - 0.10f) / 0.60f; // 0.0 to 1.0
    float scale = 0.20f + std::pow(lungeNorm, 2.5f) * 1.70f; // Accelerates directly into lens

    // Violent screen shake: random displacement up to 12 pixels
    int shakeX = static_cast<int>(FastRand(rng) % 25) - 12;
    int shakeY = static_cast<int>(FastRand(rng) % 25) - 12;

    float targetFaceW = m_maskWidth * scale;
    float targetFaceH = m_maskHeight * scale;
    float faceCenterX = (w * 0.5f) + shakeX;
    float faceCenterY = (h * 0.52f) + shakeY;

    float faceLeft = faceCenterX - targetFaceW * 0.5f;
    float faceTop = faceCenterY - targetFaceH * 0.5f;

    int chromShift = (int)(lungeNorm * 18.0f);

    for (int y = 0; y < h; ++y)
    {
        int rowOffset = y * w * 4;
        float faceNormY = (y - faceTop) / targetFaceH;

        for (int x = 0; x < w; ++x)
        {
            int idx = rowOffset + x * 4;
            float faceNormX = (x - faceLeft) / targetFaceW;

            // Base webcam sample with chromatic shake
            int redSampleX = std::clamp(x + chromShift + shakeX, 0, w - 1);
            int blueSampleX = std::clamp(x - chromShift - shakeX, 0, w - 1);
            int sampleY = std::clamp(y + shakeY, 0, h - 1);

            uint8_t bgB = src[sampleY * w * 4 + blueSampleX * 4 + 0];
            uint8_t bgG = src[sampleY * w * 4 + x * 4 + 1];
            uint8_t bgR = src[sampleY * w * 4 + redSampleX * 4 + 2];

            // Darken room around the lunging monster
            float vignetteDarkness = 1.0f - lungeNorm * 0.65f;
            bgB = (uint8_t)(bgB * vignetteDarkness);
            bgG = (uint8_t)(bgG * vignetteDarkness);
            bgR = (uint8_t)(bgR * vignetteDarkness);

            // Composite creature face if within bounds
            if (faceNormX >= 0.0f && faceNormX < 1.0f && faceNormY >= 0.0f && faceNormY < 1.0f)
            {
                int maskX = (int)(faceNormX * m_maskWidth);
                int maskY = (int)(faceNormY * m_maskHeight);
                maskX = std::clamp(maskX, 0, m_maskWidth - 1);
                maskY = std::clamp(maskY, 0, m_maskHeight - 1);

                int maskIdx = (maskY * m_maskWidth + maskX) * 4;
                uint8_t cB = m_creatureFaceMask[maskIdx + 0];
                uint8_t cG = m_creatureFaceMask[maskIdx + 1];
                uint8_t cR = m_creatureFaceMask[maskIdx + 2];
                uint8_t cA = m_creatureFaceMask[maskIdx + 3];

                if (cA > 0)
                {
                    float alpha = (cA / 255.0f) * (std::min)(1.0f, lungeNorm * 2.0f);
                    bgB = (uint8_t)(bgB * (1.0f - alpha) + cB * alpha);
                    bgG = (uint8_t)(bgG * (1.0f - alpha) + cG * alpha);
                    bgR = (uint8_t)(bgR * (1.0f - alpha) + cR * alpha);
                }
            }

            dst[idx + 0] = bgB;
            dst[idx + 1] = bgG;
            dst[idx + 2] = bgR;
            dst[idx + 3] = 255;
        }
    }
}

// ============================================================================
// EFFECT 3: THE HAUNTED ROOM ANOMALY
// The room's electrical power seemingly dies, plunging the scene into an eerie,
// desaturated cold infrared twilight. Spectral shadowy apparitions glide across
// the background, then an electrical surge snaps everything back to normal.
// ============================================================================
void HorrorEffects::RenderHauntedAnomaly(const uint8_t* src, uint8_t* dst, int w, int h, float progress)
{
    // Progress breakdown (2.4s total):
    // 0.0 - 0.20: Lights flicker and drop into cold infrared twilight
    // 0.20 - 0.75: Spectral shadows glide across ceiling/walls + analog noise
    // 0.75 - 1.00: Fluorescent bulb double-flash pop -> clean return to normal

    float darkLevel = 0.0f;
    float flicker = 1.0f;
    float spectralAlpha = 0.0f;

    uint32_t rng = (uint32_t)(progress * 80000.0f) + 54321u;

    if (progress < 0.20f)
    {
        float p = progress / 0.20f;
        darkLevel = p * 0.75f;
        flicker = 0.7f + 0.3f * std::sin(progress * 70.0f); // 15Hz rapid flicker
        spectralAlpha = p * 0.3f;
    }
    else if (progress < 0.75f)
    {
        float p = (progress - 0.20f) / 0.55f;
        darkLevel = 0.75f;
        flicker = 0.88f + 0.12f * std::sin(progress * 40.0f);
        spectralAlpha = 0.65f + 0.25f * std::sin(p * 3.14159f);
    }
    else
    {
        // Light reignition surge (fluorescent tube pop)
        float p = (progress - 0.75f) / 0.25f;
        if (p < 0.3f)
        {
            // First flash
            flicker = 1.8f;
            darkLevel = 0.1f;
        }
        else if (p < 0.5f)
        {
            // Brief dip
            flicker = 0.4f;
            darkLevel = 0.5f;
        }
        else
        {
            // Final recovery
            float rec = (p - 0.5f) / 0.5f;
            darkLevel = (1.0f - rec) * 0.5f;
            flicker = 1.0f;
        }
        spectralAlpha = (1.0f - p) * 0.4f;
    }

    // Shadowy spectral apparition gliding along the ceiling/background
    float ghostProgress = progress;
    float ghostX = w * (0.15f + ghostProgress * 0.65f);
    float ghostY = h * (0.22f + std::sin(ghostProgress * 6.28f) * 0.06f);
    float ghostRadiusX = w * 0.14f;
    float ghostRadiusY = h * 0.18f;

    for (int y = 0; y < h; ++y)
    {
        int rowOffset = y * w * 4;
        float dyGhost = (y - ghostY) / ghostRadiusY;

        for (int x = 0; x < w; ++x)
        {
            int idx = rowOffset + x * 4;

            uint8_t srcB = src[idx + 0];
            uint8_t srcG = src[idx + 1];
            uint8_t srcR = src[idx + 2];

            // 1. Color Grading: Desaturate into cold cyan/infrared nightmare palette
            int luma = (srcR * 77 + srcG * 150 + srcB * 29) >> 8;

            // Cold infrared grading: Blue boosted, Red crushed
            int gradedB = (int)(luma * 1.15f * flicker);
            int gradedG = (int)(luma * 0.95f * flicker);
            int gradedR = (int)(luma * 0.65f * flicker);

            // Contrast stretch / crushed shadows
            gradedB = (int)((gradedB - 25) * 1.25f);
            gradedG = (int)((gradedG - 25) * 1.25f);
            gradedR = (int)((gradedR - 25) * 1.15f);

            // Blend between original and haunted color grade
            uint8_t outB = ClampByte((int)(srcB * (1.0f - darkLevel) + gradedB * darkLevel));
            uint8_t outG = ClampByte((int)(srcG * (1.0f - darkLevel) + gradedG * darkLevel));
            uint8_t outR = ClampByte((int)(srcR * (1.0f - darkLevel) + gradedR * darkLevel));

            // 2. Composite spectral shadow apparition gliding across the upper scene
            if (spectralAlpha > 0.05f)
            {
                float dxGhost = (x - ghostX) / ghostRadiusX;
                float dGhost = dxGhost * dxGhost + dyGhost * dyGhost;
                if (dGhost < 1.0f)
                {
                    float gFactor = std::pow(1.0f - dGhost, 1.2f) * spectralAlpha;
                    // Crushes background into deep unnatural dark apparition
                    outB = (uint8_t)(outB * (1.0f - gFactor * 0.85f));
                    outG = (uint8_t)(outG * (1.0f - gFactor * 0.85f));
                    outR = (uint8_t)(outR * (1.0f - gFactor * 0.85f));
                }
            }

            // 3. Subtle analog high-ISO film grain in the dark areas
            if (darkLevel > 0.3f && (FastRand(rng) % 100 < 22))
            {
                int grain = (int)((FastRand(rng) % 31) - 15);
                outB = ClampByte(outB + grain);
                outG = ClampByte(outG + grain);
                outR = ClampByte(outR + grain);
            }

            dst[idx + 0] = outB;
            dst[idx + 1] = outG;
            dst[idx + 2] = outR;
            dst[idx + 3] = 255;
        }
    }
}
