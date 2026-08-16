# ReStrike Camera Controller rules.mk

MCU = atmega32u4
BOOTLOADER = caterina

# Build Options
BOOTMAGIC_ENABLE = yes      # Enable Bootmagic for hardware recovery
MOUSEKEY_ENABLE = yes       # Mouse keys for analog stick cursor/pan control
EXTRAKEY_ENABLE = yes       # Audio controls and System keys
CONSOLE_ENABLE = no         # Console for debug (disable to save ROM)
COMMAND_ENABLE = no         # Commands for debug
BACKLIGHT_ENABLE = no       # Custom matrix backlighting disabled
RGBLIGHT_ENABLE = yes       # WS2812 Addressable RGB LED control
ENCODER_ENABLE = yes        # Rotary encoders (Zoom + Shuffle)
OLED_ENABLE = yes           # I2C OLED display driver
OPT_DEFS += -DOLED_DISPLAY_128X64 -DOLED_IC=1 -DOLED_COLUMN_OFFSET=2 -DF_SCL=100000UL
LTO_ENABLE = yes            # Link-time optimization to fit ATmega32U4 32KB flash
POINTING_DEVICE_ENABLE = no # Enabled conditionally if analog mouse mode is preferred
JOYSTICK_ENABLE = no        # Enabled conditionally if DirectInput joystick mode is preferred
RAW_ENABLE = yes            # Bidirectional USB HID communication with reStrikeOBS plugin

# Analog ADC driver for 2-axis joystick
SRC += analog.c
