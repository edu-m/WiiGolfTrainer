#ifndef KOKESHI_DRAW_MATH_HPP
#define KOKESHI_DRAW_MATH_HPP

#include "GXTypes.h"

#include <cmath>
namespace drawing {
struct Vec3 {
    float x, y, z;
    Vec3() : x(0), y(0), z(0) {}
    Vec3(float px, float py, float pz = 0) : x(px), y(py), z(pz) {}
};

struct Rect {
    float x, y, width, height;
    Rect(float px = 0, float py = 0, float w = 0, float h = 0)
        : x(px), y(py), width(w), height(h) {}
};

inline float Clamp01(float x) {
    return x >= 0 ? (x <= 1 ? x : 1) : 0;
}

inline Vec3 Lerp(const Vec3& a, const Vec3& b, float t) {
    return Vec3(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t,
                a.z + (b.z - a.z) * t);
}

inline bool Project(const float camera[3][4], const float projection[7],
                    float width, float height, const Vec3& world, Vec3& out) {
    float v[3];
    for (int i = 0; i < 3; ++i) {
        v[i] = camera[i][0] * world.x + camera[i][1] * world.y +
               camera[i][2] * world.z + camera[i][3];
    }
    const bool ortho = projection[0] != 0;
    const float w = ortho ? 1.0f : -v[2];
    if (!(w > 0.00001f) || !(width > 0) || !(height > 0))
        return false;
    const float x =
        (projection[1] * v[0] + projection[2] * (ortho ? 1 : v[2])) / w;
    const float y =
        (projection[3] * v[1] + projection[4] * (ortho ? 1 : v[2])) / w;
    const float z = (projection[5] * v[2] + projection[6]) / w;
    if (!(x >= -1 && x <= 1 && y >= -1 && y <= 1 && z >= -1 && z <= 0))
        return false;
    out = Vec3((x + 1) * width * 0.5f, (1 - y) * height * 0.5f, z);

    return true;
}

static inline GXColor ShiftColorPhase(const GXColor c, float shiftDegrees) {
    GXColor out;
    out.a = c.a;

    float r = c.r / 255.0f;
    float g = c.g / 255.0f;
    float b = c.b / 255.0f;

    float max = (r > g) ? ((r > b) ? r : b) : ((g > b) ? g : b);
    float min = (r < g) ? ((r < b) ? r : b) : ((g < b) ? g : b);
    float delta = max - min;

    if (delta < 1e-5f) {
        out.r = c.r;
        out.g = c.g;
        out.b = c.b;
        return out;
    }

    float h;
    if (max == r) {
        h = 60.0f * fmodf(((g - b) / delta), 6.0f);
    } else if (max == g) {
        h = 60.0f * (((b - r) / delta) + 2.0f);
    } else {
        h = 60.0f * (((r - g) / delta) + 4.0f);
    }

    if (h < 0.0f) {
        h += 360.0f;
    }

    h = fmodf(h + shiftDegrees, 360.0f);
    if (h < 0.0f) 
        h += 360.0f;

    float s = delta / max;
    float v = max;

    float c_val = v * s;
    float h_prime = h / 60.0f;
    float x = c_val * (1.0f - fabsf(fmodf(h_prime, 2.0f) - 1.0f));
    float m = v - c_val;

    float r_new, g_new, b_new;
    int sextant = (int)h_prime;

    switch (sextant) {
    case 0:
        r_new = c_val;
        g_new = x;
        b_new = 0.0f;
        break;
    case 1:
        r_new = x;
        g_new = c_val;
        b_new = 0.0f;
        break;
    case 2:
        r_new = 0.0f;
        g_new = c_val;
        b_new = x;
        break;
    case 3:
        r_new = 0.0f;
        g_new = x;
        b_new = c_val;
        break;
    case 4:
        r_new = x;
        g_new = 0.0f;
        b_new = c_val;
        break;
    default:
        r_new = c_val;
        g_new = 0.0f;
        b_new = x;
        break;
    }

    out.r = (unsigned char)((r_new + m) * 255.0f + 0.5f);
    out.g = (unsigned char)((g_new + m) * 255.0f + 0.5f);
    out.b = (unsigned char)((b_new + m) * 255.0f + 0.5f);

    return out;
}

static inline GXColor AssignSpeedColor(GXColor c, float speed, float max) {
    const float n = Clamp01(speed / max);
    c.r *= n;
    c.g *= n;
    c.b *= n;
    return c;
}

} // namespace drawing
#endif
