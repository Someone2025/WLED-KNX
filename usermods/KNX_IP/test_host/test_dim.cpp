// Host test harness: segment GAs (incl. segment 0 option), start/stop dimming, segment switching
#define private public
#include "usermod_knx_ip.h"
#undef private
#include "src/dependencies/network/Network.h"
#include <cassert>
#include <cstdio>

HardwareSerial Serial; NetworkClass Network; WiFiClass WiFi;
static unsigned long g_now = 1000;
unsigned long millis(){ return g_now; } void delay(unsigned long){} void yield(){} float temperatureRead(){return 40;}
bool NetworkClass::isConnected(){return false;} bool NetworkClass::isEthernet(){return false;} IPAddress NetworkClass::localIP(){return {};} void NetworkClass::localMAC(uint8_t*){}
int WiFiClass::status(){return 0;} IPAddress WiFiClass::localIP(){return {};}
byte bri=0, effectCurrent=0, currentPreset=0, errorFlag=0, briLast=200, colPri[4];
uint16_t transitionDelay=750;
void setTimeFromAPI(uint32_t){} void stateUpdated(byte){} void colorUpdated(byte){} bool applyPreset(byte,byte){return true;}
char serverDescription[33]="test"; unsigned long ntpLastSyncTime=0;
static Segment segs[3];
uint8_t WS2812FX::getSegmentsNum() const {return 3;}
Segment& WS2812FX::getSegment(uint8_t i){return segs[i];}
Segment& WS2812FX::getMainSegment(){return segs[0];}
void WS2812FX::setBrightness(uint8_t,bool){} uint8_t WS2812FX::getModeCount() const {return 100;}
WS2812FX strip;

static void run(KnxIpUsermod &um, unsigned ms){ for(unsigned t=0;t<ms;t+=10){ g_now+=10; um.processDimRamps(); } }
static void send(uint16_t ga, uint8_t v){ auto cb=KNX.callbackFor(ga); assert(cb); cb(ga,DptMain::DPT_3xx,KnxService::GroupValue_Write,&v,1); }
static void send1(uint16_t ga, uint8_t v, DptMain d){ auto cb=KNX.callbackFor(ga); assert(cb); cb(ga,d,KnxService::GroupValue_Write,&v,1); }

int main(){
  for(int i=0;i<3;i++){ segs[i].start=0; segs[i].stop=10; segs[i].on=true; segs[i].opacity=255; segs[i].cct=127; segs[i].colors[0]=RGBW32(0,0,0,200); }
  KnxIpUsermod um;
  // Room "Kind 1": central GAs in 3/1/x, sections every 10 sub-addresses
  strcpy(um.gaInPower,"3/1/1");  strcpy(um.gaOutPower,"3/1/101");
  strcpy(um.gaInBri,"3/1/2");    strcpy(um.gaOutBri,"3/1/102");
  strcpy(um.gaInFx,"3/1/3");     strcpy(um.gaOutFx,"3/1/103");
  strcpy(um.gaInBriRel,"3/1/4"); strcpy(um.gaInCctRel,"3/1/5");
  strcpy(um.gaInCct,"3/1/6");    strcpy(um.gaOutCct,"3/1/106");
  strcpy(um.gaInWW,""); strcpy(um.gaInCW,""); strcpy(um.gaInW,""); strcpy(um.gaInRGB,"");
  strcpy(um.gaOutWW,""); strcpy(um.gaOutCW,""); strcpy(um.gaOutW,""); strcpy(um.gaOutRGB,"");
  um.segmentOffsetL=0; um.segmentOffsetM=0; um.segmentOffsetN=10;
  um.segmentGaFromOne=true;
  um.kelvinMin=2700; um.kelvinMax=6500;
  um.dimTimeMs=1000; um.cctDimTimeMs=1000; um.dimMinPct=1;

  // --- A) GA derivation with segment_ga_start_at_1: segment 0 has its OWN GAs
  assert(um.calculateSegmentGA(um.gaInPower,0)==knxMakeGroupAddress(3,1,11));
  assert(um.calculateSegmentGA(um.gaInPower,2)==knxMakeGroupAddress(3,1,31));
  assert(!um.hasGAConflicts(3));
  um.registerSegmentKOs();                 // existing power/bri/fx + new objects
  assert(um.numSegments==3);
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,11)));   // seg0 power
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,14)));   // seg0 bri_rel
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,15)));   // seg0 cct_rel
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,24)));   // seg1 bri_rel
  assert(um._segKOs[0].out[KnxIpUsermod::SK_PWR]==knxMakeGroupAddress(3,1,111));
  um.registerCentralReadResponders(); um.registerDimmingKOs();
  // central bri_rel registered by setup() in the real firmware; emulate:
  KNX.addGroupObject(knxMakeGroupAddress(3,1,4),DptMain::DPT_3xx,false,true);
  KNX.onGroup(knxMakeGroupAddress(3,1,4),[&](uint16_t,DptMain,KnxService s,const uint8_t*p,uint8_t l){ if(s==KnxService::GroupValue_Write&&l>=1) um.onKnxBrightnessRel(p[0]); });

  // --- B) central start/stop dimming from OFF
  bri=0;
  send(knxMakeGroupAddress(3,1,4), 0x09);          // increase 100 %
  assert(bri==3);                                    // switched on at minimum (1 % -> 3/255)
  assert(strip.getTransition()==0);                  // transition disabled while ramping
  run(um, 500);                                      // half the ramp time
  assert(bri>110 && bri<140);
  send(knxMakeGroupAddress(3,1,4), 0x08);          // STOP (release)
  uint8_t held=bri; run(um, 300);
  assert(bri==held);                                 // really stopped
  assert(strip.getTransition()==750);                // transition restored
  send(knxMakeGroupAddress(3,1,4), 0x01);          // decrease 100 %
  run(um, 2000);
  assert(bri==3);                                    // stops at minimum, does not switch off
  printf("B ok: bri ramp up/stop/down -> %u\n", bri);

  // --- C) small steps (cyclic senders): 25 % step ends by itself
  bri=128; send(knxMakeGroupAddress(3,1,4), 0x0B);  // increase 25 % (code 3 -> 63)
  run(um, 2000); assert(bri==191);
  printf("C ok: 25%% step -> %u\n", bri);

  // --- D) central colour temperature ramp affects ALL segments
  for(int i=0;i<3;i++) segs[i].cct=0;
  send(knxMakeGroupAddress(3,1,5), 0x09);          // colder
  run(um, 400); send(knxMakeGroupAddress(3,1,5), 0x08);
  assert(segs[0].cct>80 && segs[0].cct<120 && segs[1].cct==segs[0].cct && segs[2].cct==segs[0].cct);
  printf("D ok: cct ramp all segments -> %u\n", segs[0].cct);

  // --- E) section switching: desk (segment 1) off, rest stays on, room status stays "on"
  bri=200; for(int i=0;i<3;i++){segs[i].on=true;segs[i].opacity=255;}
  send1(knxMakeGroupAddress(3,1,21), 0, DptMain::DPT_1xx);   // seg1 power off
  assert(!segs[1].on && segs[0].on && segs[2].on);
  assert(um.roomLit());
  uint8_t b[3];
  assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_PWR,b)==1 && b[0]==0);
  // all sections off -> room status off, central ON switches all back on
  send1(knxMakeGroupAddress(3,1,11),0,DptMain::DPT_1xx); send1(knxMakeGroupAddress(3,1,31),0,DptMain::DPT_1xx);
  assert(!um.roomLit());
  um.onKnxPower(true);
  assert(segs[0].on && segs[1].on && segs[2].on && um.roomLit());
  // section ON while room is OFF switches the master on, other sections untouched
  um.onKnxPower(false); assert(bri==0);
  segs[2].on=false;
  send1(knxMakeGroupAddress(3,1,31),1,DptMain::DPT_1xx);
  assert(bri==briLast && segs[2].on);
  printf("E ok: section switching\n");

  // --- F) segment dimming (desk) from off, then stop
  segs[1].on=false;
  send(knxMakeGroupAddress(3,1,24), 0x09);
  assert(segs[1].on && segs[1].opacity==3);
  run(um, 300); send(knxMakeGroupAddress(3,1,24), 0x08);
  assert(segs[1].opacity>60 && segs[1].opacity<100);
  printf("F ok: segment dim -> %u\n", segs[1].opacity);

  // --- G) original scheme still works: segment 0 == central, not double-registered
  um.segmentGaFromOne=false;
  assert(um.calculateSegmentGA(um.gaInPower,0)==knxMakeGroupAddress(3,1,1));
  assert(!um.hasGAConflicts(3));
  um.registerSegmentKOs();
  assert(um._segKOs[0].out[KnxIpUsermod::SK_PWR]==0);   // published centrally
  // conflict detection: base1 with offset that collides with a central GA must be flagged
  um.segmentGaFromOne=true; um.segmentOffsetN=4;          // seg0 power = 3/1/5 == cct_rel central
  assert(um.hasGAConflicts(3));
  printf("G ok: original scheme + conflict detection\n");

  // --- H) HSV start/stop ramps (hue wraps, saturation clamps, STOP holds)
  um.segmentOffsetN=10; um.colorDimTimeMs=1000;
  for(int i=0;i<3;i++){ segs[i].on=true; segs[i].opacity=255; segs[i].colors[0]=RGBW32(255,0,0,10); }  // red, hue 0
  um.onKnxHueRel(0x09);                       // hue up, full turn
  run(um, 250);                               // ~90 deg -> greenish-yellow
  um.onKnxHueRel(0x08);                       // STOP
  { uint32_t c=segs[0].colors[0]; float h,s2,v2; KnxIpUsermod::rgbToHsv(R(c),G(c),B(c),h,s2,v2);
    assert(h>70 && h<110 && W(c)==10);        // white channel preserved
    run(um,300); uint32_t c2=segs[0].colors[0]; assert(c2==c); printf("H ok: hue %.0f\n",h); }
  um.onKnxHueRel(0x01);                       // hue down (wraps below 0)
  run(um, 500); um.onKnxHueRel(0x08);
  { uint32_t c=segs[0].colors[0]; float h,s2,v2; KnxIpUsermod::rgbToHsv(R(c),G(c),B(c),h,s2,v2);
    assert(h>250 && h<290); printf("H ok: wrapped hue %.0f\n",h); }
  um.onKnxSatRel(0x01); run(um, 2000);        // saturation down to 0 -> white-ish (r==g==b)
  { uint32_t c=segs[0].colors[0]; assert(R(c)==G(c) && G(c)==B(c)); printf("S ok\n"); }
  um.onKnxHSVRel(0x0B,0,0); run(um,100); um.onKnxHSVRel(0,0,0); assert(!um._dimHue.active);
  printf("ALL DIM TESTS PASSED\n");
  return 0;
}
