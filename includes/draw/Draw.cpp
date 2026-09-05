#include "Draw.hpp"

#include <egg/gfx/eggDrawGX.h>
#include <egg/gfx/eggStateGX.h>

#include <nw4r/math.h>

#if !defined(PACK_SPORTS)
#error Wii Sports US Rev 1 only
#endif

namespace drawing {
namespace {
bool sCanvasActive = false;
void Emit(const Vec3& p) {
    GXPosition3f32(p.x, p.y, p.z);
}
u8 LineWidth(float pixels) {
    if (!(pixels >= 1.0f / 6.0f))
        return 1;
    if (pixels >= 42.5f)
        return 255;
    return static_cast<u8>(pixels * 6 + 0.5f);
}
} // namespace

GXColor Color(u8 r, u8 g, u8 b, u8 a) {
    GXColor c = {r, g, b, a};
    return c;
}

u8 CurrentView() {
    return *reinterpret_cast<const u8*>(0x804BF615);
}
bool IsPass(u8 view, RPGrpRenderer::EDrawPass pass) {
    return CurrentView() == view && RPGrpRenderer::GetDrawPass() == pass;
}

Canvas::Canvas(Space space, float width, float height)
    : mValid(false), mSpace(space), mWidth(0), mHeight(0) {
    RPGrpRenderer* renderer = RPGrpRenderer::GetCurrent();
    RPGrpScreen* activeScreen = RPGrpRenderer::GetActiveScreen();
    const u8 viewId = CurrentView();
    if (sCanvasActive || renderer == NULL || activeScreen == NULL ||
        viewId >= 32)
        return;
    const u8* view = *reinterpret_cast<const u8* const*>(
        reinterpret_cast<const u8*>(renderer) + 0x14 + viewId * 4);
    if (view == NULL)
        return;
    sCanvasActive = true;
    mValid = true;
    GXGetProjectionv(mSavedProjection);
    GXGetViewportv(mSavedViewport);
    GXGetScissor(&mSavedScissor[0], &mSavedScissor[1], &mSavedScissor[2],
                 &mSavedScissor[3]);

    EGG::Screen screen(*activeScreen);
    const int projectionMode = *reinterpret_cast<const int*>(view + 8);
    if (projectionMode == 0)
        screen.SetProjectionType(EGG::Frustum::PROJ_PERSP);
    else if (projectionMode == 1)
        screen.SetProjectionType(EGG::Frustum::PROJ_ORTHO);
    screen.SetDirty(true);
    screen.SetProjectionGX();
    float viewport[6];
    GXGetViewportv(viewport);
    mWidth = viewport[2];
    mHeight = viewport[3];
    // RPGrpRenderer::Rendering copies the active camera here each view
    const float (*camera)[4] = reinterpret_cast<const float (*)[4]>(0x8040A8C0);
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 4; ++c)
            mCamera[r][c] = camera[r][c];
    GXGetProjectionv(mProjection);
    if (space == Overlay) {
        mWidth = width > 0 ? width : mWidth;
        mHeight = height > 0 ? height : mHeight;
        if (mWidth > 0 && mHeight > 0)
            SetOverlay(mWidth, mHeight);
    }
}

Canvas::~Canvas() {
    if (!mValid)
        return;
    EGG::StateGX::resetGX();
    EGG::StateGX::GXSetProjectionv_(mSavedProjection);
    EGG::StateGX::GXSetViewport_(mSavedViewport[0], mSavedViewport[1],
                                 mSavedViewport[2], mSavedViewport[3],
                                 mSavedViewport[4], mSavedViewport[5]);
    EGG::StateGX::GXSetScissor_(mSavedScissor[0], mSavedScissor[1],
                                mSavedScissor[2], mSavedScissor[3]);
    GXLoadPosMtxImm(mCamera, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
    sCanvasActive = false;
}

void Canvas::SetOverlay(float width, float height) {
    float projection[7] = {1, 2 / width, -1, -2 / height, 1, -1, 0};
    EGG::StateGX::GXSetProjectionv_(projection);
    Mtx identity;
    PSMTXIdentity(identity);
    GXLoadPosMtxImm(identity, GX_PNMTX0);
    GXSetCurrentMtx(GX_PNMTX0);
}

bool Canvas::ProjectPoint(const Vec3& world, Vec3& pixel) const {
    return mValid && mSpace == World &&
           Project(mCamera, mProjection, mWidth, mHeight, world, pixel);
}

void Canvas::Prepare(const Style& style) {
    EGG::DrawGX::BeginDrawLine(EGG::DrawGX::COLOR_CHANNEL_1,
                               EGG::DrawGX::ZMODE_0);
    GXSetZMode(mSpace == World && style.depth == DepthTest, GX_LEQUAL,
               GX_FALSE);
    GXSetBlendMode(GX_BM_BLEND, GX_BL_SRCALPHA, GX_BL_INVSRCALPHA, GX_LO_CLEAR);
    GXSetCullMode(GX_CULL_NONE);
    GXSetChanMatColor(GX_COLOR0A0, style.color);
    GXSetLineWidth(LineWidth(style.width), 0);
    GXSetCurrentMtx(GX_PNMTX0);
    if (mSpace == World)
        GXLoadPosMtxImm(mCamera, GX_PNMTX0);
}

void Canvas::Line(const Vec3& a, const Vec3& b, const Style& style) {
    if (!mValid || !(mWidth > 0 && mHeight > 0))
        return;
    Prepare(style);
    GXBegin(GX_LINES, GX_VTXFMT0, 2);
    Emit(a);
    Emit(b);
    GXEnd();
}

void Canvas::Polyline(const Vec3* points, unsigned count, const Style& style,
                      unsigned stride) {
    Strip(points, count, style, stride, false, 0);
}
void Canvas::MapPath(const Vec3* points, unsigned count, float y,
                     const Style& style, unsigned stride) {
    Strip(points, count, style, stride, true, y);
}
void Canvas::Strip(const Vec3* points, unsigned count, const Style& style,
                   unsigned stride, bool flatten, float y) {
    if (!mValid || !(mWidth > 0 && mHeight > 0) || points == NULL ||
        count < 2 || stride == 0)
        return;
    Prepare(style);
    const unsigned last = count - 1;
    const unsigned samples = last / stride + (last % stride != 0 ? 1 : 0) + 1;
    for (unsigned start = 0; start < samples - 1;) {
        const unsigned n = samples - start > 256 ? 256 : samples - start;
        GXBegin(GX_LINESTRIP, GX_VTXFMT0, static_cast<u16>(n));
        for (unsigned j = 0; j < n; ++j) {
            const unsigned sample = start + j;
            const unsigned index =
                sample == samples - 1 ? last : sample * stride;
            const Vec3& p = points[index];
            GXPosition3f32(p.x, flatten ? y : p.y, p.z);
        }
        GXEnd();
        start += n - 1;
    }
}

void Canvas::Triangle(const Vec3& a, const Vec3& b, const Vec3& c,
                      const Style& style) {
    if (!mValid || !(mWidth > 0 && mHeight > 0))
        return;
    Prepare(style);
    GXBegin(GX_TRIANGLES, GX_VTXFMT0, 3);
    Emit(a);
    Emit(b);
    Emit(c);
    GXEnd();
}
void Canvas::FillRect(const Rect& r, const Style& style) {
    if (!mValid || !(mWidth > 0 && mHeight > 0) || r.width <= 0 ||
        r.height <= 0)
        return;
    Prepare(style);
    GXBegin(GX_QUADS, GX_VTXFMT0, 4);
    Emit(Vec3(r.x, r.y));
    Emit(Vec3(r.x + r.width, r.y));
    Emit(Vec3(r.x + r.width, r.y + r.height));
    Emit(Vec3(r.x, r.y + r.height));
    GXEnd();
}
void Canvas::StrokeRect(const Rect& r, const Style& style) {
    if (r.width <= 0 || r.height <= 0)
        return;
    Vec3 p[5] = {Vec3(r.x, r.y), Vec3(r.x + r.width, r.y),
                 Vec3(r.x + r.width, r.y + r.height), Vec3(r.x, r.y + r.height),
                 Vec3(r.x, r.y)};
    Polyline(p, 5, style);
}
void Canvas::Circle(const Vec3& c, float radius, const Style& style,
                    unsigned segments) {
    Ring(c, radius, style, segments, false);
}
void Canvas::CircleXZ(const Vec3& c, float radius, const Style& style,
                      unsigned segments) {
    Ring(c, radius, style, segments, true);
}
void Canvas::Ring(const Vec3& c, float radius, const Style& style,
                  unsigned segments, bool xz) {
    if (!mValid || !(mWidth > 0 && mHeight > 0) || !(radius > 0))
        return;
    if (segments < 3)
        segments = 3;
    if (segments > 128)
        segments = 128;
    Prepare(style);
    GXBegin(GX_LINESTRIP, GX_VTXFMT0, static_cast<u16>(segments + 1));
    for (unsigned i = 0; i <= segments; ++i) {
        float sn, cs;
        nw4r::math::SinCosFIdx(&sn, &cs,
                               (i == segments ? 0 : i) * (256.0f / segments));
        Emit(Vec3(c.x + radius * cs, c.y + (xz ? 0 : radius * sn),
                  c.z + (xz ? radius * sn : 0)));
    }
    GXEnd();
}
void Canvas::WireBox(const Vec3& min, const Vec3& max, const Style& style) {
    if (!mValid || !(mWidth > 0 && mHeight > 0))
        return;
    Vec3 p[8];
    for (unsigned i = 0; i < 8; ++i)
        p[i] = Vec3(i & 1 ? max.x : min.x, i & 2 ? max.y : min.y,
                    i & 4 ? max.z : min.z);
    Prepare(style);
    GXBegin(GX_LINES, GX_VTXFMT0, 24);
    for (unsigned i = 0; i < 8; ++i)
        for (unsigned bit = 1; bit <= 4; bit *= 2)
            if ((i & bit) == 0) {
                Emit(p[i]);
                Emit(p[i | bit]);
            }
    GXEnd();
}
void Canvas::Cross(const Vec3& c, float r, const Style& style) {
    if (!(r > 0))
        return;
    Line(Vec3(c.x - r, c.y, c.z), Vec3(c.x + r, c.y, c.z), style);
    Line(Vec3(c.x, c.y - r, c.z), Vec3(c.x, c.y + r, c.z), style);
    Line(Vec3(c.x, c.y, c.z - r), Vec3(c.x, c.y, c.z + r), style);
}
void Canvas::Marker(const Vec3& world, float r, GXColor color) {
    Vec3 p;
    if (!(r > 0) || !ProjectPoint(world, p))
        return;
    SetOverlay(mWidth, mHeight);
    mSpace = Overlay;
    Vec3 diamond[5] = {Vec3(p.x, p.y - r), Vec3(p.x + r, p.y),
                       Vec3(p.x, p.y + r), Vec3(p.x - r, p.y),
                       Vec3(p.x, p.y - r)};
    Polyline(diamond, 5, Style(Color(0, 0, 0), 4, AlwaysVisible));
    Polyline(diamond, 5, Style(color, 2, AlwaysVisible));
    mSpace = World;
    EGG::StateGX::GXSetProjectionv_(mProjection);
    GXLoadPosMtxImm(mCamera, GX_PNMTX0);
}
} // namespace drawing
