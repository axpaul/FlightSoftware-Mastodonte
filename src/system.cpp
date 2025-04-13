// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "system.h"
#include "sequencer.h"

Adafruit_NeoPixel rgb(1, PIN_LED_WS2812, NEO_GRB + NEO_KHZ800);

timestamp_t compute_timestamp(absolute_time_t timestamp) {
    int64_t total_us = to_us_since_boot(timestamp);
  
    timestamp_t result;
    result.minutes      = total_us / 60000000;
    result.seconds      = (total_us % 60000000) / 1000000;
    result.milliseconds = (total_us % 1000000) / 1000;
    result.microseconds = total_us % 1000;
  
    return result;
  }

float battery_read_voltage(uint16_t raw) {
    float vRef = 3.3;                        // Tension de référence de l’ADC
    float adcVoltage = (raw / 4095.0) * vRef; // Tension mesurée par l’ADC
    float batteryVoltage = adcVoltage * 6.0; // Recalcule Vbat via le pont diviseur
    
    return batteryVoltage;
}
  
void setup_pin(void){
  pinMode(PIN_LED_STATUS, OUTPUT);
  pinMode(PIN_VCC_BAT, INPUT);
  pinMode(PIN_SMITCH_N2, INPUT_PULLUP);  // Avec pull-up, utile si bouton vers GND
}

void setup_interrupt(void){
  attachInterrupt(digitalPinToInterrupt(PIN_SMITCH_N2), seq_arm_rbf, CHANGE);
}

void setup_rgb(void){
  rgb.begin();
  rgb.setBrightness(50);
  rgb.clear();
  rgb.setPixelColor(0, rgb.Color(0, 255, 0));
  rgb.show();
}
