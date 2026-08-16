#pragma once

/* Vial Unique Keyboard ID */
#define VIAL_KEYBOARD_UID {0x8C, 0x51, 0x7E, 0x2A, 0x4B, 0x9D, 0x33, 0xFA}

/* Security unlock combo: Hold CAM 1 (Row 0, Col 0) and CAM 2 (Row 0, Col 1) */
#define VIAL_UNLOCK_COMBO_ROWS { 0, 0 }
#define VIAL_UNLOCK_COMBO_COLS { 0, 1 }

/* Number of dynamic layers stored in EEPROM */
#define DYNAMIC_KEYMAP_LAYER_COUNT 4
#define DYNAMIC_KEYMAP_MACRO_COUNT 16
#define DYNAMIC_KEYMAP_EEPROM_MAX_ADDR 1023
