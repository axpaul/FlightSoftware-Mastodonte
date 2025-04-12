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

float battery_read_voltage(uint16_t raw);
void RBF_arm();
void setup_pin();
void setup_interrupt();

void setup() {

  int debugConnected = debug_begin();
  analogReadResolution(12);

  if (debugConnected) {
    debug_println("[BOOT] Mastodonte ready.");
    debug_printf("[BOOT] Board: %s, FW: %s\n", BOARD_NAME_SYS, FW_VERSION);
    debug_printf("[BOOT] Voltage Battery : %f /n", battery_read_voltage(analogRead(PIN_VCC_BAT)));
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
    debug_println("[INTERRUPT] RBF removed");
    rgb.setPixelColor(0, rgb.Color(255, 255, 255));
    rgb.show();
    setBuzzer(true, 200, 100, 300);
    triggerRBF = 0;
  } else if (triggerRBF == 2) {
    debug_println("[INTERRUPT] RBF inserted");
    rgb.setPixelColor(0, rgb.Color(255, 0, 0));
    rgb.show();
    setBuzzer(false);
    triggerRBF = 0;
  }
  // Boucle principale sans action si pas de changement
}

float battery_read_voltage(uint16_t raw) {
  float vRef = 3.3;                        // Tension de référence de l’ADC
  float adcVoltage = (raw / 4095.0) * vRef; // Tension mesurée par l’ADC
  float batteryVoltage = adcVoltage * 6.0; // Recalcule Vbat via le pont diviseur
  return batteryVoltage;
}

void RBF_arm() {
  debug_println("[INTERRUPT] Detect");
  debug_printf("[INTERRUPT] Pin value : %d /n", digitalRead(PIN_SMITCH_N2));
  
  if (digitalRead(PIN_SMITCH_N2) == 1){
    triggerRBF = 1;
    debug_println("[INTERRUPT] High");
  }
    
  if (digitalRead(PIN_SMITCH_N2) == 0){
    triggerRBF = 2;
    debug_println("[INTERRUPT] Low");
  }
}

void setup_pin(){
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_VCC_BAT, INPUT);
  pinMode(PIN_SMITCH_N2, INPUT_PULLUP);  // Avec pull-up, utile si bouton vers GND
}

void setup_interrupt(){
  attachInterrupt(digitalPinToInterrupt(PIN_SMITCH_N2), RBF_arm, CHANGE);
}