#pragma once

#include <cstdint>
#include <vector>
#include <chrono>
#include <string>

enum class HorrorEffectType
{
    NONE = 0,
    EDGE_LURKER,     // Effect 1: Disturbing presence peeking from screen edge
    SUDDEN_LUNGER,   // Effect 2: Violent creature lunging directly into camera lens
    HAUNTED_ANOMALY  // Effect 3: Room lighting fails, haunted cold infrared anomaly
};

class HorrorEffects
{
public:
    HorrorEffects();
    ~HorrorEffects();

    // Trigger an effect by type (starts timer, auto-reverts on completion)
    void TriggerEffect(HorrorEffectType type);

    // Immediately restore NORMAL mode (emergency cut)
    void RestoreNormal();

    // Query active state
    bool IsActive() const;
    HorrorEffectType GetCurrentEffect() const { return m_currentEffect; }
    std::string GetCurrentEffectName() const;
    float GetProgress() const; // 0.0f to 1.0f

    // Process frame: takes live physical webcam BGRA frame, applies active effect in-place
    // If no effect is active, dstBgra is identical to srcBgra
    void ProcessFrame(const uint8_t* srcBgra, uint8_t* dstBgra, int width, int height);

private:
    HorrorEffectType m_currentEffect;
    std::chrono::steady_clock::time_point m_startTime;
    float m_durationSeconds;

    // Temporal frame buffer for ghostly trailing (Effect 3)
    std::vector<uint8_t> m_ghostHistoryBuffer;
    int m_historyWidth;
    int m_historyHeight;

    // Precomputed procedural texture masks (generated once at startup)
    std::vector<uint8_t> m_creatureFaceMask; // Alpha mask for jumpscare face
    int m_maskWidth;
    int m_maskHeight;

    void GenerateCreatureFace(int w, int h);

    // Individual effect pipelines
    void RenderEdgeLurker(const uint8_t* src, uint8_t* dst, int w, int h, float progress);
    void RenderSuddenLunger(const uint8_t* src, uint8_t* dst, int w, int h, float progress);
    void RenderHauntedAnomaly(const uint8_t* src, uint8_t* dst, int w, int h, float progress);
};
