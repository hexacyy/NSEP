// ─── input.cpp — HW joystick, buttons, and web jog ──────────────────────────
#include "input.h"
#include "arm.h"
#include "globals.h"

// ── HW joystick state ───────────────────────────────────────────────────────
uint8_t  joyMode    = 0;
int      joyXCenter = 2048;
int      joyYCenter = 2048;
bool     joyActive  = false;
uint32_t joyLastMs  = 0;

const uint8_t PAIR[3][2]   = { {0,1}, {2,3}, {4,5} };
const char*   PAIR_NAME[3] = { "Base+Shoulder", "Elbow+Wrist Pitch", "Wrist Roll+Gripper" };

// ── Web jog state ───────────────────────────────────────────────────────────
int      webJog[6]    = {0,0,0,0,0,0};
float    webJogAcc[6] = {0,0,0,0,0,0};
uint32_t lastWebJogMs = 0;
bool     webJogActive = false;

// ── Button debounce timers
static uint32_t lastSW = 0, lastRec = 0, lastPlay = 0, lastClr = 0, lastCycle = 0;

// ── HW joystick ─────────────────────────────────────────────────────────────
void calibrateJoystick() {
    long xs = 0, ys = 0;
    const int N = 64;
    for (int i = 0; i < N; i++) {
        xs += analogRead(JOY_X_PIN);
        ys += analogRead(JOY_Y_PIN);
        delay(5);
    }
    joyXCenter = xs / N;
    joyYCenter = ys / N;
    Serial.printf("[JOY] Center X=%d Y=%d\n", joyXCenter, joyYCenter);
}

void processJoystick() {
    if (millis() - joyLastMs < JOY_INTERVAL) return;
    joyLastMs = millis();

    int xRaw = analogRead(JOY_X_PIN);
    int yRaw = analogRead(JOY_Y_PIN);

    float xD = 0, yD = 0;
    if (abs(xRaw - joyXCenter) > JOY_DEADZONE)
        xD = (xRaw - joyXCenter) / (float)joyXCenter * JOY_SPEED;
    if (abs(yRaw - joyYCenter) > JOY_DEADZONE)
        yD = (yRaw - joyYCenter) / (float)joyYCenter * JOY_SPEED;

    uint8_t jA = PAIR[joyMode][0];
    uint8_t jB = PAIR[joyMode][1];
    joyActive = (xD != 0.0f || yD != 0.0f);

    if (xD != 0.0f) {
        joyF[jA] = constrain(joyF[jA] + xD, (float)joints[jA].lo, (float)joints[jA].hi);
        int a = (int)joyF[jA];
        if (a != joints[jA].cur) sendPWM(jA, a);
    }
    if (yD != 0.0f) {
        joyF[jB] = constrain(joyF[jB] + yD, (float)joints[jB].lo, (float)joints[jB].hi);
        int b = (int)joyF[jB];
        if (b != joints[jB].cur) sendPWM(jB, b);
    }
}

// ── Buttons ─────────────────────────────────────────────────────────────────
static bool btnPressed(uint8_t pin, uint32_t& last) {
    if (digitalRead(pin) == LOW && millis() - last > DEBOUNCE_MS) {
        last = millis(); return true;
    }
    return false;
}

void processButtons() {
    if (btnPressed(JOY_SW_PIN,    lastSW))    { joyMode = (joyMode + 1) % 3; Serial.printf("[JOY] %s\n", PAIR_NAME[joyMode]); pendingBroadcast = true; }
    if (btnPressed(BTN_REC_PIN,   lastRec))   recordPose();
    if (btnPressed(BTN_PLAY_PIN,  lastPlay))  isPlaying ? stopPlayback() : startPlayback();
    if (btnPressed(BTN_CLR_PIN,   lastClr))   clearRecording();
    if (btnPressed(BTN_CYCLE_PIN, lastCycle)) isCycling ? stopCycle()    : startCycle();
}

// ── Web jog ─────────────────────────────────────────────────────────────────
void processWebJog() {
    if (isPlaying || isCycling) { webJogActive = false; return; }
    if (millis() - lastWebJogMs < WEB_JOG_INTERVAL) return;
    lastWebJogMs = millis();

    bool moved = false, anyActive = false;
    for (int j = 0; j < 6; j++) {
        if (webJog[j] == 0) { webJogAcc[j] = 0; continue; }
        anyActive = true;
        webJogAcc[j] += (webJog[j] / 100.0f) * WEB_JOG_SPEED;
        int delta = (int)webJogAcc[j];
        if (delta == 0) continue;
        int target = constrain(joints[j].cur + delta, joints[j].lo, joints[j].hi);
        if (target == joints[j].cur) { webJogAcc[j] = 0; continue; }
        webJogAcc[j] -= (target - joints[j].cur);
        sendPWM(j, target);
        joyF[j] = (float)target;
        moved = true;
    }
    webJogActive = anyActive;
    if (moved) pendingBroadcast = true;
}
