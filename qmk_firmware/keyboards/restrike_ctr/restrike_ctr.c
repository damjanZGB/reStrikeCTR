#include "restrike_ctr.h"
#include "restrike_protocol.h"
#include "gpio.h"
#include "analog.h"
#include <string.h>

#ifdef RAW_ENABLE
#include "raw_hid.h"
#ifndef RAW_EPSIZE
#define RAW_EPSIZE 32
#endif
#endif

// ─── Global State (shared with keymap) ───
int16_t  joy_x_val       = 512;
int16_t  joy_y_val       = 512;
bool     joy_btn_state   = false;
bool     obs_connected   = false;   // True when reStrikeOBS is actively communicating

static uint32_t last_joy_poll     = 0;
static uint32_t last_heartbeat    = 0;
static uint32_t last_joy_hid_send = 0;
static uint32_t boot_time         = 0;

// ─── Raw HID Upstream Helpers ───
#ifdef RAW_ENABLE

// Send a 32-byte packet upstream to reStrikeOBS
static void restrike_send_packet(uint8_t cmd, const uint8_t *payload, uint8_t payload_len) {
    uint8_t data[RAW_EPSIZE];
    memset(data, 0, RAW_EPSIZE);
    data[0] = cmd;
    if (payload && payload_len > 0) {
        uint8_t copy_len = (payload_len > (RAW_EPSIZE - 1)) ? (RAW_EPSIZE - 1) : payload_len;
        memcpy(&data[1], payload, copy_len);
    }
    raw_hid_send(data, RAW_EPSIZE);
}

// Send device capability handshake to host
void restrike_send_handshake(void) {
    uint8_t payload[14];
    memset(payload, 0, sizeof(payload));
    payload[0]  = RESTRIKE_CTR_DEVICE_TYPE;
    payload[1]  = RESTRIKE_CTR_HW_VERSION;
    payload[2]  = RESTRIKE_CTR_FW_VERSION;
    payload[3]  = RESTRIKE_CTR_NUM_KEYS;
    payload[4]  = RESTRIKE_CTR_NUM_ENCODERS;
    payload[5]  = RESTRIKE_CTR_NUM_LEDS;
    payload[6]  = RESTRIKE_CTR_HAS_JOYSTICK;
    payload[7]  = RESTRIKE_CTR_HAS_OLED;
    payload[8]  = MATRIX_ROWS;
    payload[9]  = MATRIX_COLS;
    payload[10] = ENCODER_RESOLUTION;
    payload[11] = get_highest_layer(layer_state);
    restrike_send_packet(CMD_HANDSHAKE, payload, sizeof(payload));
}

// Send periodic heartbeat with uptime and current page
void restrike_send_heartbeat(void) {
    uint32_t uptime = timer_elapsed32(boot_time) / 1000; // seconds
    uint8_t payload[5];
    payload[0] = (uptime >> 0)  & 0xFF;  // LE byte 0
    payload[1] = (uptime >> 8)  & 0xFF;  // LE byte 1
    payload[2] = (uptime >> 16) & 0xFF;  // LE byte 2
    payload[3] = (uptime >> 24) & 0xFF;  // LE byte 3
    payload[4] = get_highest_layer(layer_state);
    restrike_send_packet(CMD_HEARTBEAT, payload, sizeof(payload));
}

// Send key press/release event upstream
void restrike_send_key_event(uint8_t key_idx, bool pressed) {
    uint8_t payload[2];
    payload[0] = key_idx;
    payload[1] = pressed ? 1 : 0;
    restrike_send_packet(CMD_KEY_EVENT, payload, sizeof(payload));
}

// Send encoder rotation event upstream
void restrike_send_encoder_event(uint8_t enc_idx, int8_t direction, uint8_t detents) {
    uint8_t payload[3];
    payload[0] = enc_idx;
    payload[1] = (uint8_t)direction;  // +1 or -1 as int8_t
    payload[2] = detents;
    restrike_send_packet(CMD_ENCODER_EVENT, payload, sizeof(payload));
}

// Send joystick analog state upstream
void restrike_send_joystick_state(int16_t x, int16_t y, bool btn) {
    uint8_t payload[5];
    payload[0] = (x >> 8) & 0xFF;   // x_hi
    payload[1] = x & 0xFF;          // x_lo
    payload[2] = (y >> 8) & 0xFF;   // y_hi
    payload[3] = y & 0xFF;          // y_lo
    payload[4] = btn ? 1 : 0;
    restrike_send_packet(CMD_JOYSTICK_STATE, payload, sizeof(payload));
}

#endif // RAW_ENABLE

// ─── Board Initialization ───

void keyboard_pre_init_kb(void) {
    // Allow OLED charge pump and power rail to stabilize on cold USB plug-in
    wait_ms(150);

    // Initialize Signal LED pin
    gpio_set_pin_output(SIG_LED_PIN);
    gpio_write_pin_low(SIG_LED_PIN);

    // Initialize Joystick Push Button (Active Low with pullup)
    gpio_set_pin_input_high(JOYSTICK_SW_PIN);

    keyboard_pre_init_user();
}

void keyboard_post_init_kb(void) {
    boot_time = timer_read32();

    // Blink signal LED on startup
    for (uint8_t i = 0; i < 3; i++) {
        gpio_write_pin_high(SIG_LED_PIN);
        wait_ms(60);
        gpio_write_pin_low(SIG_LED_PIN);
        wait_ms(60);
    }

    #ifdef RAW_ENABLE
    restrike_send_handshake();
    #endif

    keyboard_post_init_user();
}

// ─── Periodic Housekeeping ───

void housekeeping_task_kb(void) {
    uint32_t now = timer_read32();

    // Poll analog joystick every 10ms
    if (timer_elapsed32(last_joy_poll) >= 10) {
        last_joy_poll = now;

        // Read ADC channels
        joy_x_val     = analogReadPin(JOYSTICK_HORIZ_PIN);
        joy_y_val     = analogReadPin(JOYSTICK_VERT_PIN);
        joy_btn_state = !gpio_read_pin(JOYSTICK_SW_PIN); // Active LOW
    }

    #ifdef RAW_ENABLE
    // Send joystick state via Raw HID every 50ms (when OBS is connected)
    if (obs_connected && timer_elapsed32(last_joy_hid_send) >= 50) {
        last_joy_hid_send = now;
        restrike_send_joystick_state(joy_x_val, joy_y_val, joy_btn_state);
    }

    // Send heartbeat every 2 seconds
    if (timer_elapsed32(last_heartbeat) >= 2000) {
        last_heartbeat = now;
        restrike_send_heartbeat();
    }
    #endif

    housekeeping_task_user();
}
