#pragma once
#include "Arduino.h"
struct NetworkClass { bool isConnected(); bool isEthernet(); IPAddress localIP(); void localMAC(uint8_t*); };
extern NetworkClass Network;
#define WL_CONNECTED 3
struct WiFiClass { int status(); IPAddress localIP(); };
extern WiFiClass WiFi;
