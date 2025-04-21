// -------------------------------------------------------------
// # Project : FlightSoftware-Mastodonte
// # File    : board.h
// # Author  : Paul Miailhe
// # Date    : 2025-04-05
// # Object  : Pin definitions and hardware constants
// -------------------------------------------------------------

#ifndef BOARD_HEADER_FILE
#define BOARD_HEADER_FILE

// === YD-RP2040 BOARD ===
#define PIN_LED_WS2812      23  // Onboard WS2812B RGB LED 
#define PIN_LED_STATUS      25  // Onboard RP2040 LED (green)
#define PIN_BUTTON_USER     24  // Onboard USR push button
#define PIN_I2C1_SDA        6   // Onboard SDA BerrySensor
#define PIN_I2C1_SCL        7   // Onboard SCL BerrySensor

// === BR-MOTOR BOARD ===
// System board :
#define PIN_BUZZER          2   // Q2 Onboard Passive buzzer pin
#define PIN_VCC_BAT         28  // J3 Voltage battery monitoring

// I2C Bus (J4) :
#define PIN_I2C0_SDA        8   // J4 PIN 1 - SDA
#define PIN_I2C0_SCL        9   // J4 PIN 2 - SCL

// UART Bus (J13) :
#define PIN_UART_TX         0   // J13 PIN 3 IN_ISO_N1 - TX
#define PIN_UART_RX         1   // J13 PIN 4 OUT_ISO_N2 - RX

// H-Bridge Driver (J6, J7, J8) :
#define IN1_M1              14  // DRV8872 PIN 3 -  H-Bridge n°1 IN1
#define IN2_M1              15  // DRV8872 PIN 2 -  H-Bridge n°1 IN2 

#define IN1_M2              16  // DRV8872 PIN 3 -  H-Bridge n°2 IN1
#define IN2_M2              17  // DRV8872 PIN 2 -  H-Bridge n°2 IN2

#define IN1_M3              18  // DRV8872 PIN 3 -  H-Bridge n°3 IN1
#define IN2_M3              19  // DRV8872 PIN 2 - H-Bridge n°3 IN2

#define NFAUT_M1            20  // DRV8872 PIN 4 - H-Bridge NFaut M1
#define NFAUT_M2            21  // DRV8872 PIN 4 - H-Bridge NFaut M2
#define NFAUT_M3            22  // DRV8872 PIN 4 - H-Bridge NFaut M3

// Input Smitch trigger (J10, J11, J12) :
#define PIN_SMITCH_N1       26  // J10 PIN 3 - 74HC14 Smitch triger 
#define PIN_SMITCH_N2       27  // J11 PIN 3 - 74HC14 Smitch triger 

#define PIN_SMITCH_N3       10  // J12 PIN 5 - 74HC14 Smitch triger 
#define PIN_SMITCH_N4       11  // J12 PIN 4 - 74HC14 Smitch triger 
#define PIN_SMITCH_N5       12  // J12 PIN 3 - 74HC14 Smitch triger 
#define PIN_SMITCH_N6       13  // J12 PIN 2 - 74HC14 Smitch triger 

// Input Octocoupler (J9, J14)
#define PIN_OCTO_N3         3   // J9 PIN 1 - 74HC14 Smitch triger
#define PIN_OCTO_N4         4   // J14 PIN 1 - 74HC14 Smitch triger

// === Board and firmware information ===
#define BOARD_NAME_SYS "YD-RP2040 Mastodonte"
#define FW_VERSION "v0.1.1"

typedef enum {PRE_FLIGHT = 0, PYRO_RDY, ASCEND, WINDOW, DEPLOY_ALGO, DEPLOY_TIMER, DESCEND, TOUCHDOWN, ERROR_SEQ} rocket_state_t;

#endif // BOARD_HEADER_FILE
