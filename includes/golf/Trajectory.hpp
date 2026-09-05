#ifndef KOKESHI_GOLF_TRAJECTORY_HPP
#define KOKESHI_GOLF_TRAJECTORY_HPP
#include "PreviewMath.hpp"

namespace golf {
class Trajectory {
public:
    enum {
        MaxSubsteps = 3600,
        MaxPoints = MaxSubsteps / 4 + 2,
        StepsPerFrame = 480
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

private:
    void Restart();
    void Step();
    void Record();
    drawing::Vec3 mPoints[MaxPoints];
    unsigned mCount, mSubsteps;
    Result mResult;
    bool mMapVisible;
    int mPowerStep, mClub;
    float mAngle, mPower, mWindAngle;
    int mWindSpeed;
    unsigned mLie;
    const void* mPlayer;
    const void* mBall;
    drawing::Vec3 mBallPos;
};
} // namespace golf
#endif
