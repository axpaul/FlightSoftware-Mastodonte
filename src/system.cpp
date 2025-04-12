// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : system.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "system.h"

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
  