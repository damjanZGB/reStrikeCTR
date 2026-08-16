#pragma once

/* Hardware Direct Pins */
#define JOYSTICK_HORIZ_PIN F5   // A2 / ADC5
#define JOYSTICK_VERT_PIN  F4   // A3 / ADC4
#define JOYSTICK_SW_PIN    D2   // RX / PD2 (Active Low)
#define SIG_LED_PIN        D3   // TX / PD3

/* WS2812 ARGB LED Configuration */
#define RGBLIGHT_HUE_STEP 8
#define RGBLIGHT_SAT_STEP 8
#define RGBLIGHT_VAL_STEP 8
#define RGBLIGHT_SLEEP

/* I2C OLED Configuration */
#define OLED_DISPLAY_128X64
#define OLED_TIMEOUT 60000
#define OLED_BRIGHTNESS 255

/* Mechanical Locking support for switch matrices */
#define LOCKING_SUPPORT_ENABLE
#define LOCKING_RESYNC_ENABLE
