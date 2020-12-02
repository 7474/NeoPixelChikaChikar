// https://github.com/m5stack/M5Stack/edit/master/examples/Unit/RGB_LED_SK6812/display_rainbow/display_rainbow.ino
/*
    Description: Control RGB LED to run rainbow light show
    Please install library before compiling:
    FastLED: https://github.com/FastLED/FastLED
*/
#include <M5Stack.h>
#include "FastLED.h"

#define Neopixel_PIN 21
#define NUM_LEDS 15

CRGB leds[NUM_LEDS];
static TaskHandle_t FastLEDshowTaskHandle = 0;
static TaskHandle_t userTaskHandle = 0;

// https://github.com/lovyan03/M5Stack_TreeView
#include <M5TreeView.h>
M5TreeView tv;

int ledCurrentNum = 0;
char ledLabelCurrent[16];
char ledLabelR[8];
char ledLabelG[8];
char ledLabelB[8];
char ledBuf[128];

void updateLeds() {
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
  FastLED.show();
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
  fill_solid(leds, NUM_LEDS, CRGB{0, 0, 0});
  // https://www.switch-science.com/catalog/5208/
  // SK6812
  // https://www.switch-science.com/catalog/6058/
  // SK6812
  // NEOPIXEL
  FastLED.addLeds<SK6812,Neopixel_PIN,GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(10);
  FastLED.show();
  xTaskCreatePinnedToCore(FastLEDshowTask, "FastLEDshowTask", 2048, NULL, 2, NULL, 1);

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
  Serial.println("loop");
  tv.update();
}

void FastLEDshowTask(void *pvParameters)
{
  for (;;) {
//    FastLED.show();
    delay(100);
    Serial.println("FastLEDshowTask");
  }
}
