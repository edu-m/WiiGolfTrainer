#ifndef KOKESHI_GOLF_HOLE_SEARCH_HPP
#define KOKESHI_GOLF_HOLE_SEARCH_HPP
#include "../draw/Math.hpp"

namespace golf {
class HoleSearch {
public:
    enum Status { Inactive, Searching, Found, NotFound, NotFoundBail };
    enum {
        Directions = 12,
        Powers = 5,
        Seeds = 2,
        Levels = 4,
        MaxTrialSteps = 400,
        StepsPerUpdate = 15
    };
    HoleSearch() {
        Reset();
    }
    void Reset();
    void Begin(int club, unsigned lie, const drawing::Vec3& origin,
               const drawing::Vec3& goal);
    void Interrupt() {
        mTrialActive = false;
    }
    void Step();
    void Witness(float power, float angle);
    Status State() const {
        return mState;
    }
    float Power() const {
        return mWinner.power;
    }
    float Angle() const {
        return mWinner.angle;
    }
    const char* Text() const;

private:
    struct Candidate {
        float power, angle, score;
        Candidate() : power(0), angle(0), score(1.0e30f) {}
    };
    void StartTrial();
    void FinishTrial();
    void PrepareRefinement();
    Status mState;
    int mClub;
    unsigned mLie, mCoarse, mSeed, mLevel, mProbe, mTrialSteps;
    bool mTrialActive;
    float mBaseAngle, mPowerDelta, mAngleDelta;
    drawing::Vec3 mGoal;
    Candidate mTrial, mBest[Seeds], mCenter, mRoundBest, mWinner;
};
} // namespace golf
#endif
