// Host test: HSV memory (hue/saturation survive V=0 and S=0) and unregistering GAs of deleted segments
#define private public
#include "usermod_knx_ip.h"
#undef private
#include "src/dependencies/network/Network.h"
#include <cassert>
#include <cmath>
#include <cstdio>

HardwareSerial Serial; NetworkClass Network; WiFiClass WiFi;
static unsigned long g_now = 1000;
unsigned long millis(){ return g_now; } void delay(unsigned long){} void yield(){} float temperatureRead(){return 40;}
bool NetworkClass::isConnected(){return false;} bool NetworkClass::isEthernet(){return false;} IPAddress NetworkClass::localIP(){return {};} void NetworkClass::localMAC(uint8_t*){}
int WiFiClass::status(){return 0;} IPAddress WiFiClass::localIP(){return {};}
byte bri=200, effectCurrent=0, currentPreset=0, errorFlag=0, briLast=200, colPri[4];
uint16_t transitionDelay=750;
void setTimeFromAPI(uint32_t){} void stateUpdated(byte){} void colorUpdated(byte){} bool applyPreset(byte,byte){return true;}
char serverDescription[33]="test"; unsigned long ntpLastSyncTime=0;
static Segment segs[3];
static uint8_t g_segCount = 3;
uint8_t WS2812FX::getSegmentsNum() const {return g_segCount;}
Segment& WS2812FX::getSegment(uint8_t i){return segs[i];}
Segment& WS2812FX::getMainSegment(){return segs[0];}
void WS2812FX::setBrightness(uint8_t,bool){} uint8_t WS2812FX::getModeCount() const {return 100;}
WS2812FX strip;

static void run(KnxIpUsermod &um, unsigned ms){ for(unsigned t=0;t<ms;t+=10){ g_now+=10; um.processDimRamps(); } }
static bool near(float a, float b, float tol){ return std::fabs(a-b) <= tol; }
static void hsvOfSeg0(KnxIpUsermod &um, float &h, float &s, float &v){
  uint32_t c = segs[0].colors[0]; um.currentHsv(R(c), G(c), B(c), h, s, v);
}

int main(){
  for(int i=0;i<3;i++){ segs[i].start=0; segs[i].stop=10; segs[i].on=true; segs[i].opacity=255; segs[i].cct=127; segs[i].colors[0]=RGBW32(0,0,0,0); }
  KnxIpUsermod um;
  strcpy(um.gaInPower,"8/1/0");  strcpy(um.gaOutPower,"8/1/10");
  strcpy(um.gaInBri,"8/1/1");    strcpy(um.gaOutBri,"8/1/11");
  strcpy(um.gaInBriRel,"8/1/2"); strcpy(um.gaInCctRel,"8/1/3");
  strcpy(um.gaInCct,"8/1/4");    strcpy(um.gaOutCct,"8/1/13");
  strcpy(um.gaInWW,""); strcpy(um.gaInCW,""); strcpy(um.gaInW,""); strcpy(um.gaInRGB,"");
  strcpy(um.gaOutWW,""); strcpy(um.gaOutCW,""); strcpy(um.gaOutW,""); strcpy(um.gaOutRGB,"");
  um.segmentOffsetL=0; um.segmentOffsetM=0; um.segmentOffsetN=20; um.segmentGaFromOne=true;
  um.kelvinMin=2700; um.kelvinMax=6500; um.colorDimTimeMs=1000;

  // ---------- A) HSV memory ----------
  um.applyHSV(200.f, 0.8f, 1.f, true);
  float h,s,v; hsvOfSeg0(um,h,s,v);
  assert(near(h,200,0.01f) && near(s,0.8f,0.001f) && near(v,1,0.001f));

  // V absolute to 0 and back: colour must come back (was: hue/sat lost -> grey)
  um.onKnxV(0.f);
  assert(R(segs[0].colors[0])==0 && G(segs[0].colors[0])==0 && B(segs[0].colors[0])==0);
  hsvOfSeg0(um,h,s,v); assert(near(h,200,0.01f) && near(s,0.8f,0.001f) && v==0);   // status keeps H/S
  um.onKnxV(1.f);
  hsvOfSeg0(um,h,s,v); assert(near(h,200,0.01f) && near(s,0.8f,0.001f));
  { uint32_t c=segs[0].colors[0]; float hh,ss,vv; KnxIpUsermod::rgbToHsv(R(c),G(c),B(c),hh,ss,vv); assert(near(hh,200,2) && near(ss,0.8f,0.02f)); }

  // S to 0 (white) and back: hue must come back (was: jumped to 0° = red)
  um.onKnxS(0.f);
  hsvOfSeg0(um,h,s,v); assert(near(h,200,0.01f) && s==0);
  um.onKnxS(1.f);
  { uint32_t c=segs[0].colors[0]; float hh,ss,vv; KnxIpUsermod::rgbToHsv(R(c),G(c),B(c),hh,ss,vv); assert(near(hh,200,2) && ss>0.98f); }

  // V ramp (4-bit dimming) down to 0, then up again: colour preserved
  um.startHsvDim(2, 0x01); run(um,1500);          // decrease 100 %
  assert(R(segs[0].colors[0])==0 && G(segs[0].colors[0])==0 && B(segs[0].colors[0])==0);
  um.startHsvDim(2, 0x09); run(um,1500);          // increase 100 %
  { uint32_t c=segs[0].colors[0]; float hh,ss,vv; KnxIpUsermod::rgbToHsv(R(c),G(c),B(c),hh,ss,vv); assert(near(hh,200,2) && ss>0.98f && vv>0.99f); }

  // A colour set from elsewhere (e.g. WLED UI) is picked up and becomes the new memory
  segs[0].colors[0]=RGBW32(255,0,0,0); segs[1].colors[0]=segs[0].colors[0]; segs[2].colors[0]=segs[0].colors[0];
  hsvOfSeg0(um,h,s,v); assert(near(h,0,0.5f) && near(s,1,0.001f));
  um.onKnxV(0.f); um.onKnxV(1.f);
  { uint32_t c=segs[0].colors[0]; assert(R(c)==255 && G(c)==0 && B(c)==0); }
  printf("HSV MEMORY OK\n");

  // ---------- B) GAs of deleted segments are unregistered ----------
  g_segCount = 3;
  um.registerSegmentKOs();
  const uint16_t seg2Pwr = um.calculateSegmentGA(um.gaInPower,2);     // 8/1/60
  const uint16_t seg2BriRel = um.calculateSegmentGA(um.gaInBriRel,2); // 8/1/62
  const uint16_t seg2PwrOut = um.calculateSegmentGA(um.gaOutPower,2); // 8/1/70
  assert(seg2Pwr==knxMakeGroupAddress(8,1,60));
  assert(KNX.hasGroupObject(seg2Pwr) && KNX.hasGroupObject(seg2BriRel) && KNX.hasGroupObject(seg2PwrOut));

  // central objects registered the normal way (as in buildKoRegistry)
  KNX.addGroupObject(knxMakeGroupAddress(8,1,0), DptMain::DPT_1xx, false, true);
  KNX.onGroup(knxMakeGroupAddress(8,1,0), [](uint16_t, DptMain, KnxService, const uint8_t*, uint8_t){});

  // config change: a central object now uses a GA that segment 2 had before (registered fresh after
  // clearRegistrations in a rebuild) -> must survive the cleanup of the old segment GAs
  const uint16_t seg2Cct = um.calculateSegmentGA(um.gaInCct,2);      // 8/1/64
  strcpy(um.gaInH,"8/1/64");
  KNX.addGroupObject(seg2Cct, DptMain::DPT_5xx, false, true);

  g_segCount = 1;                                  // user deleted two segments in WLED
  um.registerSegmentKOs();
  assert(um.numSegments==1);
  assert(!KNX.hasGroupObject(seg2Pwr) && !KNX.callbackFor(seg2Pwr));
  assert(!KNX.hasGroupObject(seg2BriRel) && !KNX.hasGroupObject(seg2PwrOut));
  assert(!KNX.hasGroupObject(um.calculateSegmentGA(um.gaInPower,1)));  // segment 1 gone too
  assert(KNX.hasGroupObject(um.calculateSegmentGA(um.gaInPower,0)));   // segment 0 still there
  assert(KNX.hasGroupObject(knxMakeGroupAddress(8,1,0)) && (bool)KNX.callbackFor(knxMakeGroupAddress(8,1,0))); // central kept

  assert(KNX.hasGroupObject(seg2Cct));             // shared with central H -> kept
  strcpy(um.gaInH,"");

  g_segCount = 3;                                  // segments added again -> GAs come back
  um.registerSegmentKOs();
  assert(KNX.hasGroupObject(seg2Pwr) && (bool)KNX.callbackFor(seg2Pwr));
  printf("STALE SEGMENT GAs OK\n");
  // ---------- C) single-channel start/stop ramps (R/G/B/W/WW/CW) ----------
  for(int i=0;i<3;i++){ segs[i].colors[0]=RGBW32(0,0,0,0); segs[i].cct=0; }
  um.colorDimTimeMs=1000;
  um.onKnxColorRel(0, 0x09);                         // R increase 100 % (button held)
  run(um,500);
  { uint8_t r=R(segs[0].colors[0]); assert(r>100 && r<160); }   // ~half way after 500 ms, not one jump
  um.onKnxColorRel(0, 0x00);                         // STOP (button released)
  uint8_t heldR=R(segs[0].colors[0]); run(um,500);
  assert(R(segs[0].colors[0])==heldR && !um._dimCh[KnxIpUsermod::CH_R].active);
  um.onKnxColorRel(0, 0x09); run(um,1500);
  assert(R(segs[0].colors[0])==255);                 // end of range
  um.onKnxColorRel(0, 0x01); run(um,1500);
  assert(R(segs[0].colors[0])==0);
  um.onKnxColorRel(1, 0x0A); run(um,1500);           // G +50 % step as short ramp
  { uint8_t g=G(segs[0].colors[0]); assert(g>=126 && g<=128); }
  // composite RGB relative: start B and G down, then all-STOP
  um.onKnxRGBRel(0x00, 0x01, 0x09); run(um,200);
  assert(um._dimCh[KnxIpUsermod::CH_B].active && um._dimCh[KnxIpUsermod::CH_G].active);
  um.onKnxRGBRel(0,0,0);
  assert(!um._dimCh[KnxIpUsermod::CH_B].active && !um._dimCh[KnxIpUsermod::CH_G].active);
  // warm white ramp: W and CCT recombined
  for(int i=0;i<3;i++){ segs[i].colors[0]=RGBW32(0,0,0,0); segs[i].cct=0; }
  um.onKnxWWRel(0x09); run(um,1500);
  assert(W(segs[0].colors[0])==255 && segs[0].cct==0);
  um.onKnxCWRel(0x0A); run(um,1500);                 // add 50 % cold: W stays clipped at 255, CCT moves cold
  assert(segs[0].cct>0);
  assert(!um._dimTransitionOverride);                // transition restored after all ramps
  printf("CHANNEL RAMPS OK\n");
  printf("ALL MEM TESTS PASSED\n");
  return 0;
}
