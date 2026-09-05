#include "includes/draw/Draw.hpp"
#include "includes/golf/Bindings.hpp"
#include "includes/golf/Trajectory.hpp"
#include <egg/gfx/eggScreen.h>

#include <libkiwi.h>

#include <revolution/OS.h>

#include <kokeshi.hpp>

#define PREVIEW 1

namespace {
using namespace drawing;

class GolfPreviewDraw : public IRPGrpDrawObject {
public:
    golf::Trajectory trajectory;
    bool paused;
    GolfPreviewDraw() : paused(false) {}

    virtual void UserDraw() {
        if (paused || !trajectory.Visible())
            return;
        const GXColor red = Color(255, 40, 40);
        const Vec3* points = trajectory.Points();
        const unsigned count = trajectory.Count();
        // ViewMain
        if (IsPass(golf::game::ViewMain)) {
            Canvas world(Canvas::World);
            world.Polyline(points, count, Style(red, 2, DepthTest), 4);
        }
        // ViewMap
        else if (IsPass(golf::game::ViewMap) && trajectory.MapVisible()) {
            Canvas map(Canvas::World);
            map.MapPath(points, count, points[0].y,
                        Style(Color(0, 0, 0, 200), 4, AlwaysVisible), 4);

            map.MapPath(points, count, points[0].y,
                        Style(red, 2, AlwaysVisible), 4);

            if (trajectory.Status() != golf::Trajectory::Computing) {
                Vec3 end = points[count - 1];
                end.y = points[0].y;
                map.Marker(end, 5,
                           trajectory.Status() == golf::Trajectory::Rest
                               ? red
                               : Color(255, 180, 40));
            }
        }
        // ViewGauge
        else if (IsPass(golf::game::ViewGauge) && golf::game::GaugeVisible()) {
            Canvas ui(Canvas::Overlay, EGG::Screen::GetSizeXMax(),
                      EGG::Screen::GetSizeYMax());
            const float x = golf::game::GaugeX();
            const float y = golf::GaugeY(trajectory.Power());
            ui.FillRect(Rect(x - 13, y - 3, 26, 6), Style(Color(0, 0, 0, 220)));
            ui.FillRect(Rect(x - 12, y - 1.5f, 24, 3), Style(red));
            ui.Triangle(Vec3(x - 18, y - 4), Vec3(x - 13, y),
                        Vec3(x - 18, y + 4), Style(red));
        }
        // ViewHud
        else if (IsPass(golf::game::ViewHud, RPGrpRenderer::EDrawPass_Draw2D)) {
            const char* status = "";
            if (trajectory.Status() == golf::Trajectory::Computing)
                status = " calculating";
            else if (trajectory.Status() == golf::Trajectory::Hazard)
                status = " hazard";
            else if (trajectory.Status() == golf::Trajectory::TimeLimit)
                status = " partial";
#if PREVIEW
            kiwi::Text("Preview %d%% | meter %d%% | %dyd%s",
                       (int)(trajectory.Scale() * 100 + 0.5f),
                       (int)(trajectory.Power() * 100 + 0.5f),
                       trajectory.DistanceYards(), status)
                .SetPosition(0.06f, 0.84f)
                .SetScale(0.7f)
                .SetTextColor(kiwi::Color::RED)
                .SetStroke(kiwi::Color::BLACK, kiwi::ETextStroke_Outline)
                .SetDrawFlags(kiwi::ETextFlag_TextLeft);
#endif
        }
    }
};

class GolfTrajectoryHook : public kiwi::ISceneHook {
public:
    GolfTrajectoryHook()
        : kiwi::ISceneHook(kiwi::ESceneID_RPGolScene), mpRenderer(NULL) {}

    virtual void AfterCalculate(RPSysScene* scene) {
#pragma unused(scene)
        RPGrpRenderer* renderer = RPGrpRenderer::GetCurrent();
        if (renderer != NULL && renderer != mpRenderer) {
            renderer->AppendDrawObject(&mDraw);
            mpRenderer = renderer;
            mDraw.trajectory.Reset();
        }
        if (!mDraw.paused && renderer != NULL)
            mDraw.trajectory.Update();
    }
    virtual void AfterReset(RPSysScene* scene) {
#pragma unused(scene)
        mDraw.trajectory.Reset();
        mDraw.paused = false;
    }
    virtual void Pause(RPSysScene* scene, bool enter) {
#pragma unused(scene)
        mDraw.paused = enter;
    }
    virtual void Exit(RPSysScene* scene) {
#pragma unused(scene)
        mpRenderer = NULL;
        mDraw.trajectory.Reset();
        mDraw.paused = false;
    }

private:
    GolfPreviewDraw mDraw;
    RPGrpRenderer* mpRenderer;
};
GolfTrajectoryHook sGolfTrajectoryHook;
} // namespace

/**
 * Mod entrypoint
 */
void KokeshiMain() {
#ifndef NDEBUG
    // Setup libkiwi debugging utilities
    kiwi::Nw4rException::CreateInstance();
    kiwi::MapFile::CreateInstance();
    kiwi::MapFile::GetInstance().Open(kokeshi::MAPFILE_PATH,
                                      kiwi::MapFile::ELinkType_Relocatable);
#endif

    // Initialize network socket system
    kiwi::LibSO::Initialize();

    kiwi::cout << "Golf trajectory preview loaded" << kiwi::endl;

    // Enter first scene
    kiwi::SceneCreator::GetInstance().ChangeBootScene();

    // Enter game loop
    RP_GET_INSTANCE(RPSysSystem)->mainLoop();

    // Main function should never return
    ASSERT(false);
}
KOKESHI_BY_PACK(KM_CALL(0x80183b6c, KokeshiMain), // Wii Sports
                KM_CALL(NULL, KokeshiMain),       // Wii Play
                KM_CALL(NULL, KokeshiMain));      // Wii Sports Resort
