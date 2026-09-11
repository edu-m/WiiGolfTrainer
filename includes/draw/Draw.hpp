#ifndef KOKESHI_DRAW_HPP
#define KOKESHI_DRAW_HPP

#include "Math.hpp"
#include <Pack/RPGraphics.h>

#include <revolution/GX.h>

namespace drawing {
enum Depth { DepthTest, AlwaysVisible };

struct Style {
    GXColor color;
    float width;
    Depth depth;
    Style(GXColor c, float w = 2, Depth d = DepthTest)
        : color(c), width(w), depth(d) {}
};

GXColor Color(u8 r, u8 g, u8 b, u8 a = 255);
u8 CurrentView();
bool IsPass(u8 view,
            RPGrpRenderer::EDrawPass pass = RPGrpRenderer::EDrawPass_3);

class Canvas {
public:
    enum Space { World, Overlay };
    explicit Canvas(Space space, float width = 0, float height = 0);
    ~Canvas();
    bool IsValid() const {
        return mValid && mWidth > 0 && mHeight > 0;
    }
    float Width() const {
        return mWidth;
    }
    float Height() const {
        return mHeight;
    }
    bool ProjectPoint(const Vec3& world, Vec3& pixel) const;

    void Line(const Vec3& a, const Vec3& b, const Style& style);
    void Polyline(const Vec3* points, unsigned count, const Style& style,
                  unsigned stride = 1, bool shift_phase = false);
    void PolylineSpeed(const Vec3* points, const float* speeds, unsigned count,
                       const Style& style, unsigned stride = 1);
    void MapPath(const Vec3* points, unsigned count, float planeY,
                 const Style& style, unsigned stride = 1);
    void Triangle(const Vec3& a, const Vec3& b, const Vec3& c,
                  const Style& style);
    void FillRect(const Rect& rect, const Style& style);
    void StrokeRect(const Rect& rect, const Style& style);
    void Circle(const Vec3& center, float radius, const Style& style,
                unsigned segments = 32);
    void CircleXZ(const Vec3& center, float radius, const Style& style,
                  unsigned segments = 32);
    void WireBox(const Vec3& min, const Vec3& max, const Style& style);
    void Cross(const Vec3& center, float radius, const Style& style);
    void Marker(const Vec3& world, float radiusPixels, GXColor color);

private:
    Canvas(const Canvas&);
    Canvas& operator=(const Canvas&);
    void Prepare(const Style& style);
    void Strip(const Vec3*, unsigned, const Style&, unsigned, bool, float);
    void StripShiftPhase(const Vec3*, unsigned, const Style&, unsigned, bool, float);
    void StripSpeed(const Vec3*, const float*, unsigned, const Style&, unsigned,
                    bool, float);
    void Ring(const Vec3&, float, const Style&, unsigned, bool);
    void SetOverlay(float width, float height);

    bool mValid;
    Space mSpace;
    float mWidth, mHeight;
    Mtx mCamera;
    float mProjection[7], mSavedProjection[7], mSavedViewport[6];
    u32 mSavedScissor[4];
};
} // namespace drawing
#endif
