#ifndef KOKESHI_GOLF_PREVIEW_MATH_HPP
#define KOKESHI_GOLF_PREVIEW_MATH_HPP
#include "../draw/Math.hpp"

namespace golf {
inline bool AngleChanged(float a, float b) {
    const float turn = 6.28318530718f;
    float delta = a - b;
    if (delta < 0)
        delta = -delta;
    if (delta > turn * 0.5f)
        delta = turn - delta;
    return delta > 0.0001f || delta < -0.0001f;
}

inline int NextPowerStep(int step) {
    return step >= 18 ? 0 : step + 1;
}
inline float PowerScale(int step) {
    return (100 - step * 5) * 0.01f;
}
// FUN_802b1aa4
inline float GaugeY(float power) {
    return 299.0f - 125.0f * drawing::Clamp01(power);
}
} // namespace golf
#endif
