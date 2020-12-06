#include <M5Stack.h>

// LCD
// 下半分UI

// NeoPixel
// https://github.com/adafruit/Adafruit_NeoPixel
#include <Adafruit_NeoPixel.h>
#define Neopixel_PIN 21
#define Neopixel_NUM_LEDS 15

Adafruit_NeoPixel pixels(Neopixel_NUM_LEDS, Neopixel_PIN, NEO_GRB + NEO_KHZ800);
struct CRGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};
CRGB leds[Neopixel_NUM_LEDS];
static TaskHandle_t LEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

#include <LEDAnimator.h>
LEDAnimator<Neopixel_NUM_LEDS> ledAnimator(Neopixel_NUM_LEDS, Neopixel_PIN, NEO_GRB + NEO_KHZ800);

// BGM
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
  M5.Lcd.fillRect(0, 0, 320, 100, 0);
  M5.Lcd.fillRect(0, 20, 80, 20, M5.Lcd.color565(
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
  M5.Lcd.drawString(ledBuf, 2, 2, 1);
  Serial.println(ledBuf);

  // Update pixels
  ledAnimator.stop();
  pixels.setPixelColor(ledCurrentNum, pixels.Color(
                         leds[ledCurrentNum].r,
                         leds[ledCurrentNum].g,
                         leds[ledCurrentNum].b));
  pixels.show();
}

// Callbask
void ledNumSel(MenuItem* mi) {
  if (ledCurrentNum + 1 < Neopixel_NUM_LEDS) {
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

void ledAnimeSel(MenuItem* mi) {
  if (ledAnimator.isPlaying()) {
    ledAnimator.stop();
  } else {
    ledAnimator.start();
  }
}

void bgmSel(MenuItem* mi) {
  int no = mi->tag;
  M5BGMPlayerTrack *selectedTrack = NULL;
  for (M5BGMPlayerTrack *tmpTrack = bgm.trackList(); tmpTrack; tmpTrack = tmpTrack->right) {
    if (tmpTrack->no == no) {
      selectedTrack = tmpTrack;
      break;
    }
  }
  if (selectedTrack) {
    bgmTrack = selectedTrack;
    bgmDesire = Play;
  }
}

// Setup
void setupM5() {
  M5.begin();
  M5.Power.begin();
  if (!SD.begin()) {
    Serial.println("Failed SD.begin()");
  }
  Serial.println("End SD.begin()");
}

LEDAnimatorFrame<Neopixel_NUM_LEDS> ssjFrames[15];
void setupNeoPixel() {
  pixels.begin();
  fill_solid(leds, Neopixel_NUM_LEDS, CRGB{0, 0, 0});

  LEDAnimatorCRGB ssj0 = LEDAnimatorCRGB{0, 0, 0};
  LEDAnimatorCRGB ssj1 = LEDAnimatorCRGB{32, 16, 0};
  LEDAnimatorCRGB ssj2 = LEDAnimatorCRGB{24, 16, 0};
  LEDAnimatorCRGB ssj3 = LEDAnimatorCRGB{8, 8, 0};
  for (int i = 0; i < 15; i++) {
    ssjFrames[i].frameType = Solid;
    ssjFrames[i].millis = 50;
    for (int j =  0; j < Neopixel_NUM_LEDS; j++) {
      switch ((Neopixel_NUM_LEDS * 15 - i + j) % 5) {
        case 0:
          ssjFrames[i].leds[j] = ssj3;
          break;
        case 1:
          ssjFrames[i].leds[j] = ssj2;
          break;
        case 2:
          ssjFrames[i].leds[j] = ssj1;
          break;
        default:
          ssjFrames[i].leds[j] = ssj0;
          break;
      }
    }
  }
  ledAnimator.setFrames(ssjFrames, 15);
  ledAnimator.printFrames();

  xTaskCreatePinnedToCore(LEDshowTask, "LEDshowTask", 2048, NULL, 2, NULL, 0);
}

void setupBgm() {
  if (!bgm.init()) {
    Serial.println("Failed bgm.init()");
  }
  Serial.println("End bgm.init()");
  if (!bgm.loadTrackListFromSD("/sounds")) {
    Serial.println("Failed bgm.loadTrackListFromSD()");
  }
  Serial.println("End bgm.loadTrackListFromSD()");

  bgmInitialized = true;

  // XXX ESP8266Audio はメインループから使わないと落ちる。なんでかは見てない（多分分からない）。
  // SDカードじゃなくてスピーカ出力のほうで落ちてる感じではあった。
  //  xTaskCreatePinnedToCore(BGMUpdateTask, "BGMUpdateTask", 2048, NULL, 2, NULL, 1);
}

void setupMenu() {
  std::vector<MenuItem*> bgmMenu;
  for (M5BGMPlayerTrack *tmpTrack = bgm.trackList(); tmpTrack; tmpTrack = tmpTrack->right) {
    bgmMenu.push_back(new MenuItem(tmpTrack->path, tmpTrack->no, bgmSel));
    Serial.println(tmpTrack->path);
  }
  std::vector<MenuItem*> ledMenu;
  ledMenu.push_back(new MenuItem("SSJ Static", 0, ledAnimeSel));
  // Menu initialization
  tv.clientRect.x = 0;
  tv.clientRect.y = 120;
  tv.clientRect.w = 320;
  tv.clientRect.h = 104;
  tv.setItems(std::vector<MenuItem*>
  {
    new MenuItem("BGM", bgmMenu),
    new MenuItem("LED", ledMenu),
    new MenuItem("Test LED", std::vector<MenuItem*>{
      new MenuItem("Sel LED", ledNumSel),
      new MenuItem("R", ledRSel),
      new MenuItem("G", ledGSel),
      new MenuItem("B", ledBSel)
    }),
  });
  tv.begin();
}

void setup() {
  setupM5();
  setupNeoPixel();
  setupBgm();
  setupMenu();

  updateLeds();
}

// Loop
void loopBgm() {
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
  }
}

void loop()
{
  tv.update();
  loopBgm();
}

// Task
void LEDshowTask(void *pvParameters)
{
  for (;;) {
    ledAnimator.update();
    delay(1);
  }
}

void BGMUpdateTask(void *pvParameters)
{
  for (;;) {
    loopBgm();
    delay(1);
  }
}
