// https://github.com/m5stack/M5Stack/edit/master/examples/Unit/RGB_LED_SK6812/display_rainbow/display_rainbow.ino
/*
    Description: Control RGB LED to run rainbow light show
    Please install library before compiling:
    FastLED: https://github.com/FastLED/FastLED
*/
#include <M5Stack.h>

// https://github.com/adafruit/Adafruit_NeoPixe
#include <Adafruit_NeoPixel.h>
#define Neopixel_PIN 21
#define NUM_LEDS 15

Adafruit_NeoPixel pixels(NUM_LEDS, Neopixel_PIN, NEO_GRB + NEO_KHZ800);
struct CRGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
CRGB leds[NUM_LEDS];
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

#include <M5BGMPlayer.h>
M5BGMPlayer bgm;

enum BGMTask {
  None, Play, Stop
};
BGMTask bgmDesire = None;
M5BGMPlayerTrack *bgmTrack = NULL;
bool bgmInitialized = false;

// https://github.com/lovyan03/M5Stack_TreeView
#include <M5TreeView.h>
M5TreeView tv;

int ledCurrentNum = 0;
char ledLabelCurrent[16];
char ledLabelR[8];
char ledLabelG[8];
char ledLabelB[8];
char ledBuf[128];

void fill_solid(CRGB* pLeds, int num, const CRGB& val) {
  while (num-- > 0) {
    *pLeds = val;
    pLeds++;
  }
}

void updateLeds() {
  // Update LCD
  M5.Lcd.fillRect(0, 120, 320, 100, 0);
  M5.Lcd.fillRect(0, 140, 80, 8, M5.Lcd.color565(
                    leds[ledCurrentNum].r,
                    leds[ledCurrentNum].g,
                    leds[ledCurrentNum].b));
  M5.Lcd.setTextColor(0xffff, 0);
  M5.Lcd.setTextSize(1);
  sprintf(ledBuf, "LED[%d](%3d, %3d, %3d)",
          ledCurrentNum,
          leds[ledCurrentNum].r,
          leds[ledCurrentNum].g,
          leds[ledCurrentNum].b);
  M5.Lcd.drawString(ledBuf, 2, 122, 1);
  Serial.println(ledBuf);

  // Update pixels
  pixels.setPixelColor(ledCurrentNum, pixels.Color(
                         leds[ledCurrentNum].r,
                         leds[ledCurrentNum].g,
                         leds[ledCurrentNum].b));
  pixels.show();
}
void ledNumSel(MenuItem* mi) {
  if (ledCurrentNum + 1 < NUM_LEDS) {
    ledCurrentNum += 1;
  } else {
    ledCurrentNum = 0;
  }
  updateLeds();
}
void ledRSel(MenuItem* mi) {
  leds[ledCurrentNum].r = (leds[ledCurrentNum].r + 16) % 256;
  updateLeds();
}
void ledGSel(MenuItem* mi) {
  leds[ledCurrentNum].g = (leds[ledCurrentNum].g + 16) % 256;
  updateLeds();
}
void ledBSel(MenuItem* mi) {
  leds[ledCurrentNum].b = (leds[ledCurrentNum].b + 16) % 256;
  updateLeds();
}

void setup() {
  M5.begin();
  M5.Power.begin();

  // Neopixel initialization
  pixels.begin();
  fill_solid(leds, NUM_LEDS, CRGB{0, 0, 0});
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, NULL, 0);

  // BGM initialization
  if (!SD.begin()) {
    Serial.println("Failed SD.begin()");
  }
  Serial.println("SD.begin()");
  if (bgm.init()) {
    Serial.println("Failed bgm.init()");
  }
  Serial.println("bgm.init()");
  if (bgm.loadTrackListFromSD("/sounds")) {
    Serial.println("Failed bgm.init()");
  }
  Serial.println("bgm.loadTrackListFromSD()");

  bgmInitialized = true;
  // XXX ESP8266Audio はメインループから使わないと落ちる。なんでかは見てない（多分分からない）。
  //  xTaskCreatePinnedToCore(BGMUpdateTask, "BGMUpdateTask", 2048, NULL, 2, NULL, 1);

  // Menu initialization
  tv.clientRect.x = 2;
  tv.clientRect.y = 2;
  tv.clientRect.w = 316;
  tv.clientRect.h = 118;
  tv.setItems(std::vector<MenuItem*>
  { new MenuItem( "NeoPixel", std::vector<MenuItem*>{
      new MenuItem( "Sel LED", ledNumSel)
      , new MenuItem( "R", ledRSel)
      , new MenuItem( "G", ledGSel)
      , new MenuItem( "B", ledBSel)
    })
  });
  tv.begin();

  updateLeds();
}

void loop()
{
  //  Serial.println("loop");
  tv.update();

  if (bgmInitialized) {
    switch (bgmDesire) {
      case Play:
        bgm.play(bgmTrack);
        break;
      case Stop:
        bgm.stop();
        break;
      default: break;
    }
    bgmDesire = None;
    bgm.update();
    //    if (!bgm.isPlaying()) {
    //      if (bgmTrack) {
    //        bgmTrack = bgmTrack->right;
    //      }
    //      if (!bgmTrack) {
    //        bgmTrack = bgm.trackList();
    //      }
    //      if (bgmTrack) {
    //        Serial.println(bgmTrack->path);
    //      }
    //      bgmDesire = Play;
    //    }
  }
}

void FastLEDshowTask(void *pvParameters)
{
  for (;;) {
    //    Serial.println("FastLEDshowTask");
    //    FastLED.show();
    delay(100);
  }
}

void BGMUpdateTask(void *pvParameters)
{
  for (;;) {
    switch (bgmDesire) {
      case Play:
        bgm.play(bgmTrack);
        break;
      case Stop:
        bgm.stop();
        break;
      default: break;
    }
    bgmDesire = None;
    bgm.update();

    delay(1);
  }
}
