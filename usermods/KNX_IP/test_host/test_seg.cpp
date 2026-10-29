// Host test harness for the per-segment colour objects (logic only, no network)
#define private public
#include "usermod_knx_ip.h"
#undef private
#include "src/dependencies/network/Network.h"
#include <cassert>
#include <vector>

HardwareSerial Serial; NetworkClass Network; WiFiClass WiFi;
unsigned long millis(){ static unsigned long t=0; return t+=10; } void delay(unsigned long){} void yield(){} float temperatureRead(){return 40;}
bool NetworkClass::isConnected(){return false;} bool NetworkClass::isEthernet(){return false;} IPAddress NetworkClass::localIP(){return {};} void NetworkClass::localMAC(uint8_t*){}
int WiFiClass::status(){return 0;} IPAddress WiFiClass::localIP(){return {};}
uint16_t transitionDelay=750;
byte bri=128, effectCurrent=0, currentPreset=0, errorFlag=0, briLast=128, colPri[4];
void setTimeFromAPI(uint32_t){}
char serverDescription[33]="test"; unsigned long ntpLastSyncTime=0; void stateUpdated(byte){} void colorUpdated(byte){} bool applyPreset(byte,byte){return true;}

// --- strip stub with 3 segments; record setCCT/setColor calls ---
static Segment segs[3];
static int lastCctSeg=-1; static uint16_t lastCctVal=0;
uint8_t WS2812FX::getSegmentsNum() const {return 3;}
Segment& WS2812FX::getSegment(uint8_t i){return segs[i];}
Segment& WS2812FX::getMainSegment(){return segs[0];}
void WS2812FX::setBrightness(uint8_t,bool){} uint8_t WS2812FX::getModeCount() const {return 100;}
WS2812FX strip;

int main(){
  for(int i=0;i<3;i++){ segs[i].start=0; segs[i].stop=10; segs[i].on=true; segs[i].opacity=255; segs[i].cct=127; segs[i].colors[0]=RGBW32(0,0,0,200); }
  KnxIpUsermod um;
  // central GAs: room "Kind 1" style plan, segment offset N=10 (sub group)
  strcpy(um.gaInCct,"3/1/20");  strcpy(um.gaOutCct,"3/1/120");
  strcpy(um.gaInWW,"3/1/21");   strcpy(um.gaOutWW,"3/1/121");
  strcpy(um.gaInCW,"3/1/22");   strcpy(um.gaOutCW,"3/1/122");
  strcpy(um.gaInW,"3/1/23");    strcpy(um.gaOutW,"3/1/123");
  strcpy(um.gaInRGB,"3/1/24");  strcpy(um.gaOutRGB,"3/1/124");
  um.segmentOffsetL=0; um.segmentOffsetM=0; um.segmentOffsetN=10;
  um.kelvinMin=2700; um.kelvinMax=6500; um.segmentGaFromOne=false; // original scheme in this test
  um.numSegments=3;
  um.registerSegmentColorKOs();

  // 1) GA derivation: segment 1 CCT IN = 3/1/30, segment 2 = 3/1/40; segment 0 not registered
  assert(um._segKOs.size()==3);
  assert(um._segKOs[0].in[KnxIpUsermod::SK_CCT]==0);
  assert(um._segKOs[1].in[KnxIpUsermod::SK_CCT]==knxMakeGroupAddress(3,1,30));
  assert(um._segKOs[2].in[KnxIpUsermod::SK_CCT]==knxMakeGroupAddress(3,1,40));
  assert(um._segKOs[1].out[KnxIpUsermod::SK_CCT]==knxMakeGroupAddress(3,1,130));
  assert(um._segKOs[0].out[KnxIpUsermod::SK_CCT]==0);
  // registered in core with callback
  assert(KNX.hasGroupObject(knxMakeGroupAddress(3,1,30)));
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,30)));
  assert((bool)KNX.callbackFor(knxMakeGroupAddress(3,1,130))); // read responder on OUT

  // 2) Kelvin write via the registered callback -> segment 1 cct mapped linearly 2700..6500
  {
    uint8_t p[2]={ (uint8_t)(4600>>8), (uint8_t)(4600&0xFF) };
    // stub setCCT is a no-op -> emulate by checking mapping helper directly + callback doesn't crash
    KNX.callbackFor(knxMakeGroupAddress(3,1,30))(0,DptMain::DPT_7xx,KnxService::GroupValue_Write,p,2);
    assert(um.kelvinToCct255(4600)==128);
    assert(um.kelvinToCct255(2700)==0 && um.kelvinToCct255(6500)==255);
  }

  // 3) encodeSegmentValue: CCT -> Kelvin big endian, WW/CW split, BRI/PWR
  {
    segs[1].cct=255; segs[1].colors[0]=RGBW32(1,2,3,200); segs[1].opacity=128;
    uint8_t b[3];
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_CCT,b)==2); assert(((b[0]<<8)|b[1])==6500);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_CW,b)==1 && b[0]==200);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_WW,b)==1 && b[0]==0);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_RGB,b)==3 && b[0]==1 && b[1]==2 && b[2]==3);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_PWR,b)==1 && b[0]==1);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_BRI,b)==1 && b[0]==KnxIpCore::packScaling(50));
    segs[1].on=false;
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_PWR,b)==1 && b[0]==0);
    assert(um.encodeSegmentValue(1,KnxIpUsermod::SK_BRI,b)==1 && b[0]==0);
    assert(um.encodeSegmentValue(7,KnxIpUsermod::SK_PWR,b)==0); // out of range -> no answer
  }

  // 4) read responder chaining keeps an existing write handler on a shared GA
  {
    uint16_t ga=knxMakeGroupAddress(5,0,1); int writes=0;
    KNX.addGroupObject(ga,DptMain::DPT_5xx,false,true);
    KNX.onGroup(ga,[&](uint16_t,DptMain,KnxService s,const uint8_t*,uint8_t){ if(s==KnxService::GroupValue_Write) writes++; });
    strcpy(um.gaOutBri,"5/0/1"); um.GA_OUT_BRI=ga; um.answerReadRequests=true;
    um.registerCentralReadResponders();
    uint8_t v=10; KNX.callbackFor(ga)(ga,DptMain::DPT_5xx,KnxService::GroupValue_Write,&v,1);
    KNX.callbackFor(ga)(ga,DptMain::DPT_5xx,KnxService::GroupValue_Read,nullptr,0); // must not reach write handler
    assert(writes==1);
  }

  // 5) conflict list contains the colour GAs when enabled
  { std::vector<const char*> l; um.appendSegmentCentralGAs(l); assert(l.size()==12);
    um.segmentColorEnabled=false; l.clear(); um.appendSegmentCentralGAs(l); assert(l.empty()); }

  printf("ALL TESTS PASSED\n");
  return 0;
}
