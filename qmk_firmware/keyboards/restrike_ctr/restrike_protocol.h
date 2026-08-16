#pragma once

/*
 * ReStrike Camera Controller - Raw HID Protocol Specification v1.0
 *
 * Bidirectional 32-byte USB HID packets between the controller firmware
 * and the reStrikeOBS plugin. This header defines the shared wire protocol
 * used by both sides.
 *
 * Packet Structure:
 *   Byte 0:    Command ID (see enums below)
 *   Byte 1-31: Payload (command-specific)
 *
 * Convention:
 *   0x01-0x7F = Downstream (OBS → Controller)
 *   0x80-0xFF = Upstream   (Controller → OBS)
 */

/* ─── Hardware Capability Descriptor (sent in HANDSHAKE) ─── */
#define RESTRIKE_CTR_HW_VERSION     1
#define RESTRIKE_CTR_FW_VERSION     1
#define RESTRIKE_CTR_NUM_KEYS       10   // 8 matrix switches + 2 encoder clicks
#define RESTRIKE_CTR_NUM_ENCODERS   2
#define RESTRIKE_CTR_NUM_LEDS       12
#define RESTRIKE_CTR_HAS_JOYSTICK   1
#define RESTRIKE_CTR_HAS_OLED       1

/* ─── Device Identity (matches USB descriptor for host-side enumeration) ─── */
#define RESTRIKE_CTR_DEVICE_TYPE    0x02  // 0x01 = D100H, 0x02 = ReStrike CTR

/* ─── Downstream Commands (OBS → Controller) ─── */
enum restrike_cmd_downstream {
    CMD_SET_ACTIVE_CAMERA   = 0x01,  // [1] cam_id (1-4)
    CMD_SET_REC_STREAM      = 0x02,  // [1] rec (0/1), [2] stream (0/1)
    CMD_SET_AUDIO_LEVELS    = 0x03,  // [1] ch1_vu, [2] ch2_vu, [3] ch3_vu, [4] ch4_vu (0-100)
    CMD_SET_STREAM_HEALTH   = 0x04,  // [1] status: 0=OK, 1=WARN, 2=CRITICAL
    CMD_SET_TALLY_COLOR     = 0x05,  // [1] led_idx, [2] r, [3] g, [4] b
    CMD_SET_OLED_LINE       = 0x06,  // [1] line (0-5), [2..21] ascii_text (20 chars max)
    CMD_SET_LED_BRIGHTNESS  = 0x07,  // [1] brightness (0-255)
    CMD_SET_KEY_MAPPING     = 0x08,  // [1] key_idx, [2] action_type, [3] param1, [4] param2
    CMD_SET_PAGE            = 0x09,  // [1] page_id (0-3)
    CMD_SET_SCENE_NAMES     = 0x0A,  // [1] cam_idx (0-3), [2..21] scene_name (20 chars)
    CMD_PING                = 0x0F,  // Keepalive ping from host
};

/* ─── Upstream Commands (Controller → OBS) ─── */
enum restrike_cmd_upstream {
    CMD_KEY_EVENT           = 0x81,  // [1] key_idx, [2] state (1=press, 0=release)
    CMD_ENCODER_EVENT       = 0x82,  // [1] enc_idx, [2] direction (+1/-1 as int8_t), [3] detent_count
    CMD_JOYSTICK_STATE      = 0x83,  // [1] x_hi, [2] x_lo, [3] y_hi, [4] y_lo, [5] btn (0/1)
    CMD_HANDSHAKE           = 0x84,  // Device capability descriptor (see below)
    CMD_HEARTBEAT           = 0x85,  // [1-4] uptime_sec (uint32_t LE), [5] active_page
    CMD_PONG                = 0x8F,  // Response to CMD_PING
};

/* ─── Handshake Payload Layout (CMD_HANDSHAKE = 0x84) ─── */
// Byte  0:   CMD_HANDSHAKE (0x84)
// Byte  1:   device_type       (RESTRIKE_CTR_DEVICE_TYPE)
// Byte  2:   hw_version        (RESTRIKE_CTR_HW_VERSION)
// Byte  3:   fw_version        (RESTRIKE_CTR_FW_VERSION)
// Byte  4:   num_keys          (RESTRIKE_CTR_NUM_KEYS)
// Byte  5:   num_encoders      (RESTRIKE_CTR_NUM_ENCODERS)
// Byte  6:   num_leds          (RESTRIKE_CTR_NUM_LEDS)
// Byte  7:   has_joystick      (0/1)
// Byte  8:   has_oled          (0/1)
// Byte  9:   matrix_rows       (MATRIX_ROWS)
// Byte 10:   matrix_cols       (MATRIX_COLS)
// Byte 11:   encoder_resolution
// Byte 12:   active_page       (current layer)
// Byte 13-31: reserved (zero-filled)

/* ─── Stream Health Status Codes ─── */
#define HEALTH_OK        0
#define HEALTH_WARNING   1
#define HEALTH_CRITICAL  2

/* ─── Key Action Types (for CMD_SET_KEY_MAPPING) ─── */
enum restrike_action_type {
    ACTION_SCENE_SWITCH     = 0x01,  // param1 = scene index
    ACTION_TOGGLE_REC       = 0x02,
    ACTION_TOGGLE_STREAM    = 0x03,
    ACTION_MUTE_SOURCE      = 0x04,  // param1 = source index
    ACTION_REPLAY_SAVE      = 0x05,
    ACTION_REPLAY_PLAY      = 0x06,
    ACTION_CUSTOM_HOTKEY    = 0x10,  // param1 = modifier, param2 = keycode
    ACTION_PAGE_CYCLE       = 0xF0,
    ACTION_NONE             = 0xFF,
};
