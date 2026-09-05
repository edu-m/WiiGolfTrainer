#ifndef KOKESHI_DRAW_MATH_HPP
#define KOKESHI_DRAW_MATH_HPP

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

} // namespace drawing
#endif
