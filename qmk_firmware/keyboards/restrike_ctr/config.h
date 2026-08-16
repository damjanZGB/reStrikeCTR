#pragma once

#include "config_common.h"

/* USB Device descriptor parameter */
#define VENDOR_ID       0xFEED
#define PRODUCT_ID      0x6060
#define DEVICE_VER      0x0001
#define MANUFACTURER    ReStrike
#define PRODUCT         ReStrike Camera Controller

/* Key matrix size */
#define MATRIX_ROWS 2
#define MATRIX_COLS 5

/* Key matrix pins */
#define MATRIX_ROW_PINS { B5, F6 }
#define MATRIX_COL_PINS { D4, C6, D7, E6, B4 }
#define DIODE_DIRECTION COL2ROW

/* Rotary Encoders */
#define ENCODERS_PAD_A { F7, B3 }
#define ENCODERS_PAD_B { B1, B2 }
#define ENCODER_RESOLUTION 4

/* Hardware Direct Pins */
#define JOYSTICK_HORIZ_PIN F5   // A2 / ADC5
#define JOYSTICK_VERT_PIN  F4   // A3 / ADC4
#define JOYSTICK_SW_PIN    D2   // RX / PD2 (Active Low)
#define SIG_LED_PIN        D3   // TX / PD3

/* WS2812 ARGB LED Configuration */
#define RGB_DI_PIN B6
#define RGBLED_NUM 12
#define RGBLIGHT_LIMIT_VAL 200
#define RGBLIGHT_HUE_STEP 8
#define RGBLIGHT_SAT_STEP 8
#define RGBLIGHT_VAL_STEP 8
#define RGBLIGHT_SLEEP

/* Enable RGB Layers for dynamic tally/layer lighting */
#define RGBLIGHT_LAYERS
#define RGBLIGHT_MAX_LAYERS 8

/* Enable specific RGB animations */
#define RGBLIGHT_EFFECT_BREATHING
#define RGBLIGHT_EFFECT_RAINBOW_MOOD
#define RGBLIGHT_EFFECT_RAINBOW_SWIRL
#define RGBLIGHT_EFFECT_SNAKE
#define RGBLIGHT_EFFECT_KNIGHT
#define RGBLIGHT_EFFECT_STATIC_GRADIENT
#define RGBLIGHT_EFFECT_RGB_TEST
#define RGBLIGHT_EFFECT_ALTERNATING
#define RGBLIGHT_EFFECT_TWINKLE

/* I2C OLED Configuration */
#define OLED_TIMEOUT 60000
#define OLED_BRIGHTNESS 255

/* Debounce filtering */
#define DEBOUNCE 5

/* Mechanical Locking support for switch matrices */
#define LOCKING_SUPPORT_ENABLE
#define LOCKING_RESYNC_ENABLE
