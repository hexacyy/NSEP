// ─── arm.h — Joint table, servo control, recording, playback ────────────────
#pragma once
#include "config.h"

// ── State (defined in arm.cpp)
extern Joint  joints[6];
extern float  joyF[6];                  // smooth target buffer (degrees)

extern Pose   seq[MAX_POSES];
extern int    seqLen;
extern bool   isPlaying;
extern bool   isCycling;
extern int    playIdx;
extern uint32_t playNextMs;

extern const int POSE_HOME [6];
extern const int POSE_READY[6];
extern const int POSE_PICK [6];
extern const int POSE_PLACE[6];

// ── Servo
uint16_t toCounts(const Joint& j, int angle);
void     setServo(uint8_t i, int angle);              // full mutexed path
void     sendPWM (uint8_t i, int angle);              // fast — caller owns bus
void     smoothMove(uint8_t i, int target, uint8_t steps = 40, uint8_t ms = 15);
void     moveToHome();
void     applyPreset(const int p[6]);

// ── Recording / playback
void recordPose();
void renamePose(int idx, const char* name);
void startPlayback();
void stopPlayback();
void startCycle();
void stopCycle();
void clearRecording();
void saveToFlash();
void loadFromFlash();
void processPlayback();
