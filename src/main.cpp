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

volatile bool triggerRBF = false;

void onInterrupt(){
   triggerRBF = true;
}

void setup() {

  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_SMITCH_N2, INPUT);  // Entrée logique (pull-up si nécessaire)

  int debugConnected = debugBegin();  

  // Active l’interruption sur front montant (LOW→HIGH)
  attachInterrupt(digitalPinToInterrupt(PIN_SMITCH_N2), onInterrupt, FALLING);

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

  setBuzzer(true, 1000, 150, 1200);

}

void loop(){

  if (triggerRBF) {
    triggerRBF = false;
    debugPrintln("[INTERUPT] RBF remove");
    rgb.Color(255, 255, 255);
    setBuzzer(true, 200, 100, 300);

  }

}
