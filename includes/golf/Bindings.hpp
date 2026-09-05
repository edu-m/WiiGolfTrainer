#ifndef KOKESHI_GOLF_BINDINGS_HPP
#define KOKESHI_GOLF_BINDINGS_HPP
#include <types.h>

#include "../draw/Math.hpp"

#if !defined(PACK_SPORTS)
#error Wii Sports US Rev 1 only
#endif

namespace golf {
namespace game {
typedef drawing::Vec3 VEC3;

struct GolfBall;

// RPGolMapObjBall field offsets
const u32 BALL_POS_PTR = 0x04C; // VEC3* mpTrans
const u32 BALL_MOVING = 0x05C;  // bool mIsMoving
const u32 BALL_BEFORE = 0x05D;  // bool mIsBeforeShot
const u32 BALL_HAZARD = 0x098;  // s32 EKillType (0 = none)
const u32 BALL_SWING = 0x138;   // void* mpSwing

// RPGolPlayer field offsets
const u32 PLYR_ANGLE = 0x3C;   // f32 aim angle (radians)
const u32 PLYR_AIMDIR = 0x40;  // f32[2] aim direction (sin, cos)
const u32 PLYR_BALLPOS = 0x54; // VEC3 current ball position
const u32 PLYR_LIE = 0x6C;     // u16 ground attribute under the ball

typedef void (*BallCalcFn)(GolfBall* pBall, int forceNonVirtual);
typedef void (*BallCopyVirtFn)(GolfBall* pDst, GolfBall* pSrc, GolfBall* pSfx);
typedef void (*BallSetupFn)(GolfBall* pBall, VEC3* pVel, f64 spin, f64 spin2);
typedef void (*BallStopFn)(GolfBall* pBall);

// club vtable slot +0x0C, computes launch velocity/spin for a swing power
typedef void (*ClubApplyVelFn)(void* pClub, const f32* pAimDir, u32 lieAttr,
                               VEC3* pVelOut, f32* pSpinOut, f64 power);

typedef VEC3* (*GetGoalPosFn)(u8* pFieldMgr);
// is the overhead map hidden? nonzero = hidden
typedef int (*MapHiddenFn)(u8* pFieldMgr, int unused);

const BallCalcFn BallCalculate = (BallCalcFn)0x80299890;
const BallCopyVirtFn BallCopyFromVirtual = (BallCopyVirtFn)0x8029A5EC;
const BallSetupFn BallSetup = (BallSetupFn)0x8029A0E0;
const BallStopFn BallStop = (BallStopFn)0x802977C8;
const GetGoalPosFn FieldGetGoalPos = (GetGoalPosFn)0x80292294;
const MapHiddenFn FieldMapHidden = (MapHiddenFn)0x8029256C;
typedef u8* (*GetCoreCtrlFn)(u8* pPlayer);
const GetCoreCtrlFn PlayerGetCoreController = (GetCoreCtrlFn)0x802A0CA8;

const u32 WPAD_MINUS = 0x1000;

const int CLUB_PT = 3; // putter

// golf singletons
inline GolfBall* SimBall() {
    return *(GolfBall**)0x804BF878; // RPGolMapObjBall::spSimStopBall
}
inline u8* SimMgr() {
    return *(u8**)0x804BF890; // RPGolSimulateManager::spInstance
}
inline u8* PlayerMgr() {
    return *(u8**)0x804BF8D0; // RPGolPlayerManager::sInstance
}
inline u8* ClubMgr() {
    return *(u8**)0x804BF900; // RPGolClubManager::spInstance
}
inline u8* FieldMgr() {
    return *(u8**)0x804BF868; // RPGolFieldManager::sInstance
}

inline u8* CurrentPlayer() {
    u8* pMgr = PlayerMgr();
    return pMgr != NULL ? *(u8**)(pMgr + 0x334) : NULL;
}
inline GolfBall* CurrentBall() {
    u8* pMgr = SimMgr();
    return pMgr != NULL ? *(GolfBall**)(pMgr + 0x6B4) : NULL;
}

template <typename T> T Read(const void* pBase, u32 offset) {
    return *(const T*)((const u8*)pBase + offset);
}

enum View {
    ViewMain = 0,
    ViewTee = 1,
    ViewGauge = 2,
    ViewHud = 3,
    ViewWind = 4,
    ViewMap = 5,
    ViewLateHud = 6
};

inline u8* CurrentSwing() {
    u8* player = CurrentPlayer();
    return player != NULL ? Read<u8*>(player, 0x8C) : NULL;
}
inline bool GaugeVisible() {
    typedef u32 (*HiddenFn)();
    return CurrentSwing() != NULL && ((HiddenFn)0x802A2710)() == 0;
}
inline f32 GaugeX() {
    return Read<f32>(CurrentSwing(), 0x44);
}

} // namespace game
} // namespace golf
#endif
