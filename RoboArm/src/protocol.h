// ─── protocol.h — JSON builders, WebSocket, and serial commands ─────────────
#pragma once
#include <ESPAsyncWebServer.h>

const char* buildStatus();
const char* buildPoses();
void        broadcastStatus();
void        broadcastPoses();

void processWsCmd(char* msg);
void onWsEvent(AsyncWebSocket*, AsyncWebSocketClient* client,
               AwsEventType type, void* arg, uint8_t* data, size_t len);

void processSerial();
