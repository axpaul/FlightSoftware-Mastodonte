// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : sequencer.cpp
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : 
// -------------------------------------------------------------

#include "board.h"
#include "sequencer.h"
#include "hardware/timer.h"

bool seq_arm_rbf(){

    bool trigger_RBF;
    timestamp_t ts = compute_timestamp(get_absolute_time());
    debug_println("[INTERRUPT] Detect : ");
    debug_printf("[INTERRUPT] T = %02lu:%02lu.%03lu.%03lu\n", ts.minutes, ts.seconds, ts.milliseconds, ts.microseconds);
    debug_printf("[INTERRUPT] Pin value : %d\n", digitalRead(PIN_SMITCH_N2));
    
    if (digitalRead(PIN_SMITCH_N2) == 0){
      return true;
      debug_println("[INTERRUPT] High");
    }
      
    if (digitalRead(PIN_SMITCH_N2) == 1){
      return false;
      debug_println("[INTERRUPT] Low");
    }
}