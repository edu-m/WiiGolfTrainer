#include "HoleSearch.hpp"

#include "Bindings.hpp"

#include <nw4r/math.h>

namespace golf {
using namespace game;

void HoleSearch::Reset() {
    mState = Inactive;
    mTrialActive = false;
    mClub = -1;
    mLie = mCoarse = mSeed = mLevel = mProbe = mTrialSteps = 0;
    mBaseAngle = mPowerDelta = mAngleDelta = 0;
    mGoal = drawing::Vec3();
    mTrial = mCenter = mRoundBest = mWinner = Candidate();
    for (unsigned i = 0; i < Seeds; ++i)
        mBest[i] = Candidate();
}

void HoleSearch::Begin(int club, unsigned lie, const drawing::Vec3& origin,
                       const drawing::Vec3& goal) {
    Reset();
    mClub = club;
    mLie = lie;
    mGoal = goal;
    mBaseAngle = atan2f(goal.x - origin.x, goal.z - origin.z);

    if (mClub == CLUB_PT) {
        mState = Searching;
        return;
    }

    void* clubPtr = Read<void*>(ClubMgr(), 0xC + mClub * 4);
    ClubApplyVelFn applyVel = (*(ClubApplyVelFn**)clubPtr)[3];
    float aim[2];
    nw4r::math::SinCosRad(&aim[0], &aim[1], mBaseAngle);
    VEC3 maxVel;
    float spin = 0;
    applyVel(clubPtr, aim, mLie, &maxVel, &spin, 1.0f);

    const float vXZ =
        nw4r::math::FSqrt(maxVel.x * maxVel.x + maxVel.z * maxVel.z);
    const float vY = maxVel.y;
    const float deltaY = origin.y - goal.y;

    // derived from RPGolMapObjBall::CalcPhysics
    const float gravity = 0.002f;

    //   deltaY + vY*t - 0.5*g*t^2 = 0
    const float discriminant = vY * vY + 2.0f * gravity * deltaY;

    if (discriminant >= 0.0f) {
        const float airtime = (vY + nw4r::math::FSqrt(discriminant)) / gravity;
        const float carry = airtime * vXZ;

        const float absoluteMaxRange = carry * 2.0f;

        const float dx = goal.x - origin.x;
        const float dz = goal.z - origin.z;
        const float distToHole = nw4r::math::FSqrt(dx * dx + dz * dz);

        if (distToHole > absoluteMaxRange) {
            mState = NotFoundBail;
            return;
        }
    }

    mState = Searching;
}

const char* HoleSearch::Text() const {
    switch (mState) {
    case Searching:
        return "searching";
    case Found:
        return "yes";
    case NotFound:
        return "not found";
    case NotFoundBail:
        return "too far!";
    default:
        return "unknown";
    }
}

void HoleSearch::Witness(float power, float angle) {
    if (mState == Found)
        return;
    mWinner.power = power;
    mWinner.angle = angle;
    mWinner.score = 0;
    mState = Found;
    mTrialActive = false;
}

void HoleSearch::PrepareRefinement() {
    mCenter = mRoundBest = mBest[mSeed];
    mLevel = mProbe = 0;
    mPowerDelta = 0.05f;
    mAngleDelta = 3.14159265359f / Directions;
}

void HoleSearch::StartTrial() {
    if (mCoarse < Directions * Powers) {
        const unsigned direction = mCoarse / Powers;
        const int offset = direction == 0
                               ? 0
                               : (direction & 1 ? int((direction + 1) / 2)
                                                : -int(direction / 2));
        mTrial.angle = mBaseAngle + offset * (6.28318530718f / Directions);
        mTrial.power = (Powers - mCoarse % Powers) / float(Powers);
    } else {
        mTrial = mCenter;
        if (mProbe == 0)
            mTrial.power += mPowerDelta;
        if (mProbe == 1)
            mTrial.power -= mPowerDelta;
        if (mProbe == 2)
            mTrial.angle += mAngleDelta;
        if (mProbe == 3)
            mTrial.angle -= mAngleDelta;
        mTrial.power = drawing::Clamp01(mTrial.power);
    }

    mTrial.score = 1.0e30f;
    mTrialSteps = 0;

    void* club = Read<void*>(ClubMgr(), 0xC + mClub * 4);
    ClubApplyVelFn applyVel = (*(ClubApplyVelFn**)club)[3];
    float aim[2];
    nw4r::math::SinCosRad(&aim[0], &aim[1], mTrial.angle);
    VEC3 velocity;
    float spin = 0;
    applyVel(club, aim, mLie, &velocity, &spin, mTrial.power);
    GolfBall* sim = SimBall();
    BallCopyFromVirtual(sim, CurrentBall(), NULL);
    BallSetup(sim, &velocity, spin, 0.0);
    *(void**)((u8*)sim + BALL_SWING) = NULL;
    mTrialActive = true;
}

void HoleSearch::FinishTrial() {
    mTrialActive = false;

    if (mCoarse < Directions * Powers) {
        for (unsigned i = 0; i < Seeds; ++i) {
            if (mTrial.score < mBest[i].score) {
                for (unsigned j = Seeds - 1; j > i; --j)
                    mBest[j] = mBest[j - 1];
                mBest[i] = mTrial;
                break;
            }
        }
        if (++mCoarse == Directions * Powers)
            PrepareRefinement();
        return;
    }

    if (mTrial.score < mRoundBest.score)
        mRoundBest = mTrial;

    if (++mProbe < 4)
        return;

    mProbe = 0;

    if (mRoundBest.score < mCenter.score) {
        mCenter = mRoundBest;
    } else { // zoom in
        mPowerDelta *= 0.5f;
        mAngleDelta *= 0.5f;
    }

    if (++mLevel < Levels)
        return;

    if (++mSeed < Seeds) {
        PrepareRefinement();
    } else {
        mState = NotFound;
    }
}

void HoleSearch::Step() {
    if (mState != Searching)
        return;

    for (unsigned i = 0; i < StepsPerUpdate && mState == Searching; ++i) {
        if (!mTrialActive)
            StartTrial();

        GolfBall* sim = SimBall();
        BallCalculate(sim, 0);
        ++mTrialSteps;

        const bool hazard = Read<int>(sim, BALL_HAZARD) != 0;
        if (!hazard && BallIsHoled(sim, mGoal)) {
            Witness(mTrial.power, mTrial.angle);
            return;
        }

        bool flybyAbort = false;
        const VEC3* pos = Read<VEC3*>(sim, BALL_POS_PTR);

        if (!hazard && pos != NULL) {
            const float dx = pos->x - mGoal.x;
            const float dy = pos->y - mGoal.y;
            const float dz = pos->z - mGoal.z;

            // penalty applied to missiles flying through the hole
            float heightPenalty = (dy > 0.5f) ? (dy * 10.0f) : 0.0f;

            const float score = dx * dx + dy * dy + dz * dz + heightPenalty;

            if (score < mTrial.score)
                mTrial.score = score;
            else if (score > mTrial.score + 15.0f && mTrialSteps > 40)
                flybyAbort = true;
        }

        if (hazard || Read<u8>(sim, BALL_MOVING) == 0 ||
            mTrialSteps >= MaxTrialSteps || flybyAbort) {

            if (hazard) {
                BallStop(sim);
                mTrial.score = 1.0e30f;
            }
            FinishTrial();
        }
    }
}
} // namespace golf