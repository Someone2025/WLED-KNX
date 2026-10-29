#pragma once
#include "Arduino.h"
#include <vector>
#include <map>
struct JsonObject;
struct JsonVariant { operator JsonObject() const;  template<class T> JsonVariant& operator=(const T&){return *this;} template<class T> T operator|(T d) const {return d;} const char* operator|(const char* d) const {return d;} bool isNull() const {return true;} JsonVariant operator[](const char*) const {return {};} };
struct JsonArray { template<class T> void add(T){} };
struct JsonObject { JsonVariant operator[](const char*) const {return {};} JsonObject createNestedObject(const char*){return {};} JsonArray createNestedArray(const char*){return {};} bool isNull() const {return true;} JsonObject& operator=(const JsonVariant&){return *this;} };
class Usermod { public: virtual ~Usermod(){} virtual void setup()=0; virtual void loop()=0; virtual void addToJsonInfo(JsonObject&){} virtual void addToConfig(JsonObject&){} virtual bool readFromConfig(JsonObject&){return true;} virtual void appendConfigData(Print&){} virtual uint16_t getId(){return 0;} virtual const char* getName(){return "";} };
#define R(c) (uint8_t((c) >> 16))
#define G(c) (uint8_t((c) >> 8))
#define B(c) (uint8_t(c))
#define W(c) (uint8_t((c) >> 24))
#define RGBW32(r,g,b,w) (uint32_t((uint8_t(w) << 24) | (uint8_t(r) << 16) | (uint8_t(g) << 8) | (uint8_t(b))))
#define CALL_MODE_DIRECT_CHANGE 1
#define SEG_OPTION_ON 2
struct Segment {
  uint32_t colors[3]; uint8_t opacity, cct, mode; bool on; uint16_t start, stop;
  bool isActive() const {return stop>start;} bool hasRGB() const {return true;} bool isCCT() const {return true;} bool hasWhite() const {return true;}
  bool isSelected() const {return true;} uint8_t getLightCapabilities() const {return 7;}
  Segment& setColor(uint8_t sl, uint32_t c){colors[sl]=c;return *this;} Segment& setCCT(uint16_t k){cct=(uint8_t)k;return *this;} Segment& setOption(uint8_t n,bool v){if(n==SEG_OPTION_ON) on=v; return *this;} Segment& setOpacity(uint8_t o){opacity=o;return *this;} Segment& setMode(uint8_t,bool=false){return *this;}
};
struct WS2812FX { uint16_t _tr=750; void setTransition(uint16_t t){_tr=t;} uint16_t getTransition() const {return _tr;} void setBrightness(uint8_t,bool=false); uint8_t getModeCount() const; uint16_t getLengthTotal() const; void trigger(); uint8_t getSegmentsNum() const; Segment& getSegment(uint8_t); Segment& getMainSegment(); };
extern WS2812FX strip;
extern byte bri, effectCurrent, currentPreset, errorFlag;
extern uint16_t transitionDelay;
void stateUpdated(byte); void colorUpdated(byte); bool applyPreset(byte, byte=0);

#define SEGCOLOR(x) strip.getSegment(0).colors[x]
extern byte briLast, colPri[4];
void setTimeFromAPI(uint32_t);

inline JsonVariant::operator JsonObject() const { return {}; }
#define REGISTER_USERMOD(x)
