// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : main.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Flight software 
// -------------------------------------------------------------

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "board.h"
#include "debug.h"
#include "buzzer.h"

Adafruit_NeoPixel rgb(1, PIN_LED_WS2812, NEO_GRB + NEO_KHZ800);

volatile uint8_t triggerRBF = 0;

void RBF_arm();

void RBF_arm() {
  debugPrintln("[INTERRUPT] Detect");
  debugPrintf("[INTERRUPT] Pin value : %d", digitalRead(PIN_SMITCH_N2));
  
  if (digitalRead(PIN_SMITCH_N2) == 1){
    triggerRBF = 1;
    debugPrintln("[INTERRUPT] High");
  }
    
  if (digitalRead(PIN_SMITCH_N2) == 0){
    triggerRBF = 2;
    debugPrintln("[INTERRUPT] Low");
  }
}

void setup() {
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_SMITCH_N2, INPUT_PULLUP);  // Avec pull-up, utile si bouton vers GND

  int debugConnected = debugBegin();

  attachInterrupt(digitalPinToInterrupt(PIN_SMITCH_N2), RBF_arm, CHANGE);

  if (debugConnected) {
    debugPrintln("[BOOT] Mastodonte ready.");
    debugPrintf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    digitalWrite(PIN_LED_STATUS, LOW);
  } else {
    digitalWrite(PIN_LED_STATUS, HIGH);
  }

  rgb.begin();
  rgb.setBrightness(50);
  rgb.clear();
  rgb.show();
}

void loop() {
  if (triggerRBF == 1) {
    debugPrintln("[INTERRUPT] RBF removed");
    rgb.setPixelColor(0, rgb.Color(255, 255, 255));
    rgb.show();
    setBuzzer(true, 200, 100, 300);
    triggerRBF = 0;
  } else if (triggerRBF == 2) {
    debugPrintln("[INTERRUPT] RBF inserted");
    rgb.setPixelColor(0, rgb.Color(255, 0, 0));
    rgb.show();
    setBuzzer(false);
    triggerRBF = 0;
  }

  // Boucle principale sans action si pas de changement
}
