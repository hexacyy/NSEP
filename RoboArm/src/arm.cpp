// ─── arm.cpp — Joint table, servo control, recording, playback ──────────────
#include "arm.h"
#include "globals.h"

// ── Joint table ─────────────────────────────────────────────────────────────
// minUs/maxUs are physical pulse-width calibration; counts derived per-tick.
Joint joints[6] = {
    //  name           ch   lo   hi  home  cur   inv   minUs maxUs
    { "Base",           5,   0, 270, 135, 135,  false,  500, 2500 },
    { "Shoulder",       0,  30, 150,  90,  90,  true,   500, 2500 },
    { "Elbow",          1,   0, 180,  90,  90,  false,  500, 2500 },
    { "Wrist Pitch",    2,   0, 180,  90,  90,  false,  500, 2500 },
    { "Wrist Roll",     3,   0, 180,  90,  90,  false,  500, 2500 },
    { "Gripper",        4,   0,  90,  45,  45,  false,  500, 2500 },
};

float joyF[6];

// ── Recording state
Pose     seq[MAX_POSES];
int      seqLen     = 0;
bool     isPlaying  = false;
bool     isCycling  = false;
int      playIdx    = 0;
uint32_t playNextMs = 0;

const int POSE_HOME [6] = { 90, 90, 90, 90, 90, 45 };
const int POSE_READY[6] = { 90, 60, 90, 90, 90, 45 };
const int POSE_PICK [6] = { 90, 45, 45, 90, 90,  0 };
const int POSE_PLACE[6] = { 90, 45, 45, 90, 90, 90 };

// ── Servo ───────────────────────────────────────────────────────────────────
uint16_t toCounts(const Joint& j, int angle) {
    long us = (long)j.minUs +
              ((long)(angle - j.lo) * ((long)j.maxUs - j.minUs)) / (j.hi - j.lo);
    long lo = min((long)j.minUs, (long)j.maxUs);
    long hi = max((long)j.minUs, (long)j.maxUs);
    return (uint16_t)(constrain(us, lo, hi) * 4096L / 20000L);
}

void setServo(uint8_t i, int angle) {
    angle = constrain(angle, joints[i].lo, joints[i].hi);
    joints[i].cur = angle;
    joyF[i]       = (float)angle;
    int phys = joints[i].invert ? (joints[i].lo + joints[i].hi - angle) : angle;
    if (xSemaphoreTake(servoMutex, pdMS_TO_TICKS(20)) == pdTRUE) {
        pca9685.setPWM(joints[i].ch, 0, toCounts(joints[i], phys));
        xSemaphoreGive(servoMutex);
    }
}

void sendPWM(uint8_t i, int angle) {
    joints[i].cur = angle;
    int phys = joints[i].invert ? (joints[i].lo + joints[i].hi - angle) : angle;
    pca9685.setPWM(joints[i].ch, 0, toCounts(joints[i], phys));
}

void smoothMove(uint8_t i, int target, uint8_t steps, uint8_t ms) {
    int start = joints[i].cur;
    target = constrain(target, joints[i].lo, joints[i].hi);
    for (uint8_t s = 1; s <= steps; s++) {
        setServo(i, start + (target - start) * s / steps);
        delay(ms);
    }
}

void moveToHome()                { for (int i = 0; i < 6; i++) setServo(i, joints[i].home); }
void applyPreset(const int p[6]) { for (int i = 0; i < 6; i++) smoothMove(i, p[i]); }

// ── Recording ───────────────────────────────────────────────────────────────
void recordPose() {
    if (seqLen >= MAX_POSES) { Serial.println("[REC] Full"); return; }
    for (int i = 0; i < 6; i++) seq[seqLen].a[i] = joints[i].cur;
    snprintf(seq[seqLen].label, sizeof(seq[seqLen].label), "Pose %d", seqLen + 1);
    seqLen++;
    Serial.printf("[REC] %d total\n", seqLen);
    pendingBroadcast = true;
}

void renamePose(int idx, const char* name) {
    if (idx < 0 || idx >= seqLen) return;
    strncpy(seq[idx].label, name, sizeof(seq[idx].label) - 1);
    seq[idx].label[sizeof(seq[idx].label) - 1] = '\0';
}

void startPlayback() {
    if (!seqLen) { Serial.println("[PLAY] Empty"); return; }
    isCycling = false; isPlaying = true;
    playIdx = 0; playNextMs = millis();
    pendingBroadcast = true;
}

void stopPlayback() { isPlaying = false; isCycling = false; pendingBroadcast = true; }

void startCycle() {
    if (!seqLen) { Serial.println("[CYCLE] Empty"); return; }
    isPlaying = false; isCycling = true;
    playIdx = 0; playNextMs = millis();
    pendingBroadcast = true;
}

void stopCycle() { isCycling = false; pendingBroadcast = true; }

void clearRecording() {
    seqLen = 0; isPlaying = false; isCycling = false; pendingBroadcast = true;
    Serial.println("[REC] Cleared");
}

void saveToFlash() {
    prefs.begin("roboarm", false);
    prefs.putInt("len", seqLen);
    if (seqLen > 0) prefs.putBytes("seq", seq, seqLen * sizeof(Pose));
    prefs.end();
    Serial.printf("[FLASH] Saved %d\n", seqLen);
}

void loadFromFlash() {
    prefs.begin("roboarm", true);
    seqLen = prefs.getInt("len", 0);
    if (seqLen > 0) prefs.getBytes("seq", seq, seqLen * sizeof(Pose));
    prefs.end();
    Serial.printf("[FLASH] Loaded %d\n", seqLen);
    pendingBroadcast = true;
}

// ── Playback ────────────────────────────────────────────────────────────────
void processPlayback() {
    if (!isPlaying && !isCycling) return;
    if (millis() < playNextMs)    return;

    if (playIdx >= seqLen) {
        if (isCycling) { playIdx = 0; }
        else {
            isPlaying = false; playIdx = 0;
            Serial.println("[PLAY] Done");
            pendingBroadcast = true;
            return;
        }
    }

    for (int i = 0; i < 6; i++) setServo(i, seq[playIdx].a[i]);
    Serial.printf("[%s] %d/%d \"%s\"\n",
        isCycling ? "CYC" : "PLAY", playIdx + 1, seqLen, seq[playIdx].label);
    playIdx++;
    playNextMs = millis() + PLAY_STEP_MS;
    pendingBroadcast = true;
}
