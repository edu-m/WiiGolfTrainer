#include "Trajectory.hpp"

#include "Bindings.hpp"

#include <nw4r/math.h>

namespace golf {
using namespace game;

Trajectory::Trajectory() {
    Reset();
}

void Trajectory::Reset() {
    mCount = mSubsteps = 0;
    mResult = Hidden;
    mMapVisible = false;
    mPowerStep = 0;
    mClub = -1;
    mAngle = mPower = mWindAngle = 0;
    mWindSpeed = 0;
    mLie = 0;
    mPlayer = mBall = NULL;
    mBallPos = VEC3();
}

void Trajectory::Update() {
    u8* player = CurrentPlayer();
    GolfBall* ball = CurrentBall();
    u8* clubs = ClubMgr();
    u8* field = FieldMgr();
    if (player == NULL || ball == NULL || clubs == NULL || field == NULL ||
        SimBall() == NULL || Read<VEC3*>(ball, BALL_POS_PTR) == NULL ||
        Read<u8>(ball, BALL_BEFORE) == 0 || Read<u8>(ball, BALL_MOVING) != 0) {
        Reset();
        return;
    }
    const int club = Read<int>(clubs, 0x1C);
    if (club < 0 || club > CLUB_PT ||
        Read<void*>(clubs, 0xC + club * 4) == NULL) {
        Reset();
        return;
    }
    const VEC3 pos = Read<VEC3>(player, PLYR_BALLPOS);
    const bool newShot = player != mPlayer || ball != mBall || club != mClub ||
                         pos.x != mBallPos.x || pos.y != mBallPos.y ||
                         pos.z != mBallPos.z;
    if (newShot)
        mPowerStep = 0;
    u8* ctrl = PlayerGetCoreController(player);
    if (ctrl != NULL && (Read<u32>(ctrl, 0x18) & WPAD_MINUS) != 0)
        mPowerStep = NextPowerStep(mPowerStep);

    float nominal = 1;
    if (club == CLUB_PT) {
        const VEC3* goal = FieldGetGoalPos(field);
        if (goal == NULL) {
            Reset();
            return;
        }
        const float dx = pos.x - goal->x, dz = pos.z - goal->z;
        nominal =
            drawing::Clamp01(nw4r::math::FSqrt(dx * dx + dz * dz) / 25.0f);
    }
    const float power = nominal * Scale();
    const float angle = Read<float>(player, PLYR_ANGLE);
    const unsigned lie = Read<u16>(player, PLYR_LIE);
    u8* wind = Read<u8*>(field, 0x68);
    const float windAngle = wind != NULL ? Read<float>(wind, 8) : 0;
    const int windSpeed = wind != NULL ? Read<int>(wind, 0x10) : 0;
    const bool changed = newShot || mResult == Hidden || power != mPower ||
                         angle != mAngle || lie != mLie ||
                         windAngle != mWindAngle || windSpeed != mWindSpeed;
    mPlayer = player;
    mBall = ball;
    mClub = club;
    mBallPos = pos;
    mPower = power;
    mAngle = angle;
    mLie = lie;
    mWindAngle = windAngle;
    mWindSpeed = windSpeed;
    mMapVisible = FieldMapHidden(field, 0) == 0;
    if (changed)
        Restart();
    if (mResult == Computing)
        Step();
}

void Trajectory::Restart() {
    u8* player = CurrentPlayer();
    void* club = Read<void*>(ClubMgr(), 0xC + mClub * 4);
    ClubApplyVelFn applyVel = (*(ClubApplyVelFn**)club)[3];
    VEC3 velocity;
    float spin = 0;
    applyVel(club, (const float*)(player + PLYR_AIMDIR), mLie, &velocity, &spin,
             mPower);
    GolfBall* sim = SimBall();
    BallCopyFromVirtual(sim, CurrentBall(), NULL);
    BallSetup(sim, &velocity, spin, 0.0);
    *(void**)((u8*)sim + BALL_SWING) = NULL;
    mCount = mSubsteps = 0;
    mResult = Computing;
    Record();
}

void Trajectory::Record() {
    const VEC3* pos = Read<VEC3*>(SimBall(), BALL_POS_PTR);
    if (pos != NULL && mCount < MaxPoints)
        mPoints[mCount++] = *pos;
}

void Trajectory::Step() {
    GolfBall* sim = SimBall();
    for (unsigned i = 0; i < StepsPerFrame; ++i) {
        BallCalculate(sim, 0);
        ++mSubsteps;
        if ((mSubsteps & 3) == 0)
            Record();
        if (Read<int>(sim, BALL_HAZARD) != 0) {
            BallStop(sim);
            mResult = Hazard;
        } else if (Read<u8>(sim, BALL_MOVING) == 0) {
            mResult = Rest;
        } else if (mSubsteps >= MaxSubsteps) {
            mResult = TimeLimit;
        }
        if (mResult != Computing) {
            if ((mSubsteps & 3) != 0)
                Record();
            break;
        }
    }
}

int Trajectory::DistanceYards() const {
    if (mCount < 2)
        return 0;
    const float dx = mPoints[mCount - 1].x - mPoints[0].x;
    const float dz = mPoints[mCount - 1].z - mPoints[0].z;
    return static_cast<int>(nw4r::math::FSqrt(dx * dx + dz * dz) * 0.10936f);
}
} // namespace golf
