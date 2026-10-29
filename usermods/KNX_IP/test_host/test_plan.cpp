#define private public
#include "usermod_knx_ip.h"
#undef private
#include "src/dependencies/network/Network.h"
#include <cassert>
#include <cstdio>
HardwareSerial Serial; NetworkClass Network; WiFiClass WiFi;
unsigned long millis(){ return 1000; } void delay(unsigned long){} void yield(){} float temperatureRead(){return 40;}
bool NetworkClass::isConnected(){return false;} bool NetworkClass::isEthernet(){return false;} IPAddress NetworkClass::localIP(){return {};} void NetworkClass::localMAC(uint8_t*){}
int WiFiClass::status(){return 0;} IPAddress WiFiClass::localIP(){return {};}
byte bri=0, effectCurrent=0, currentPreset=0, errorFlag=0, briLast=200, colPri[4]; uint16_t transitionDelay=750;
void setTimeFromAPI(uint32_t){} void stateUpdated(byte){} void colorUpdated(byte){} bool applyPreset(byte,byte){return true;}
char serverDescription[33]="t"; unsigned long ntpLastSyncTime=0;
static Segment segs[4];
uint8_t WS2812FX::getSegmentsNum() const {return 4;}
Segment& WS2812FX::getSegment(uint8_t i){return segs[i];} Segment& WS2812FX::getMainSegment(){return segs[0];}
void WS2812FX::setBrightness(uint8_t,bool){} uint8_t WS2812FX::getModeCount() const {return 100;}
WS2812FX strip;
int main(){
  for(int i=0;i<4;i++){segs[i].start=0;segs[i].stop=10;}
  KnxIpUsermod um;
  strcpy(um.gaInPower,"8/5/0"); strcpy(um.gaInBri,"8/5/1"); strcpy(um.gaInBriRel,"8/5/2"); strcpy(um.gaInCct,"8/5/3"); strcpy(um.gaInCctRel,"8/5/4"); strcpy(um.gaInPreset,"8/5/5");
  strcpy(um.gaOutPower,"8/5/10"); strcpy(um.gaOutBri,"8/5/11"); strcpy(um.gaOutCct,"8/5/13");
  um.segmentOffsetL=0; um.segmentOffsetM=0; um.segmentOffsetN=20; um.segmentGaFromOne=true;
  assert(!um.hasGAConflicts(4));
  assert(um.calculateSegmentGA(um.gaInPower,0)==knxMakeGroupAddress(8,5,20));
  assert(um.calculateSegmentGA(um.gaOutCct,1)==knxMakeGroupAddress(8,5,53));
  assert(um.calculateSegmentGA(um.gaInCctRel,1)==knxMakeGroupAddress(8,5,44));
  um.numSegments=4; um.registerSegmentKOs();
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(8,5,42)));  // seg1 dim rel
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(8,5,30)));  // seg0 status read responder
  printf("GA PLAN OK\n"); return 0; }
