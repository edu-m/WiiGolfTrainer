#ifndef KOKESHI_GOLF_TRAJECTORY_HPP
#define KOKESHI_GOLF_TRAJECTORY_HPP
#include "PreviewMath.hpp"
#include "HoleSearch.hpp"

namespace golf {
class Trajectory {
public:
    enum {
        MaxSubsteps = 3600,
        SampleInterval = 16,
        MaxPoints = MaxSubsteps / SampleInterval + 2,
        StepsPerFrame = 480,
        StepsWhileChanging = 64
    };
    enum Result { Hidden, Computing, Rest, Hazard, TimeLimit };
    Trajectory();
    void Reset();
    void Update();
    const drawing::Vec3* Points() const {
        return mPoints;
    }
    unsigned Count() const {
        return mCount;
    }
    const float* Speeds() const { return mSpeeds; }
    const float* StepSpeeds() const { return mStepSpeeds; }
    unsigned StepSpeedCount() const {
        return mResult == Hidden ? 0 : mSubsteps + 1;
    }
    bool Visible() const {
        return mResult != Hidden && mCount >= 2;
    }
    bool MapVisible() const {
        return Visible() && mMapVisible;
    }
    Result Status() const {
        return mResult;
    }
    float Power() const {
        return mPower;
    }
    float Scale() const {
        return PowerScale(mPowerStep);
    }
    int DistanceYards() const;
    const HoleSearch& Holeability() const { return mHoleSearch; }

private:
    void Restart();
    void Step(unsigned budget);
    void Record();
    void RecordSpeed();
    void UpdateDistance();
    drawing::Vec3 mPoints[MaxPoints];
    float mSpeeds[MaxPoints];
    float mStepSpeeds[MaxSubsteps + 1];
    unsigned mCount, mSubsteps;
    int mDistanceYards;
    Result mResult;
    bool mMapVisible;
    int mPowerStep, mClub;
    float mAngle, mPower, mWindAngle;
    int mWindSpeed;
    unsigned mLie;
    const void* mPlayer;
    const void* mBall;
    drawing::Vec3 mBallPos;
    drawing::Vec3 mGoalPos;
    const void* mField;
    HoleSearch mHoleSearch;
};
} // namespace golf
#endif
