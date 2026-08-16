#include QMK_KEYBOARD_H
#include "restrike_protocol.h"
#if defined(VIA_ENABLE)
#include "dynamic_keymap.h"
#endif

#ifdef RAW_ENABLE
#include "raw_hid.h"
#ifndef RAW_EPSIZE
#define RAW_EPSIZE 32
#endif
#endif

// â”€â”€â”€ External symbols from restrike_ctr.c â”€â”€â”€
extern bool obs_connected;
extern void restrike_send_handshake(void);
extern void restrike_send_key_event(uint8_t key_idx, bool pressed);
extern void restrike_send_encoder_event(uint8_t enc_idx, int8_t direction, uint8_t detents);

// 4 Distinct Controller Pages (Layers)
enum custom_pages {
    _PAGE_BROADCAST = 0, // Page 1: OBS / vMix / ReStrike Camera Switcher
    _PAGE_AUDIO,         // Page 2: Audio Mixer & Channel Mutes
    _PAGE_REPLAY,        // Page 3: Instant Replay & Timeline Editor
    _PAGE_LIGHTING       // Page 4: Full Hardware RGB Lighting & System
};

// Custom Keycodes
enum custom_keycodes {
    PAGE_CYCLE = SAFE_RANGE,
    PAGE_PREV,
    CAM_1,
    CAM_2,
    CAM_3,
    CAM_4,
    REC_TOGGLE,
    STREAM_TOGGLE,
    AUD_MUTE_CH1,
    AUD_MUTE_CH2,
    AUD_MUTE_CH3,
    AUD_MUTE_CH4,
    MARK_IN,
    MARK_OUT,
    SAVE_REPLAY,
    SPEED_25,
    SPEED_50,
    SPEED_75,
    SPEED_100,
    RGB_TALLY_TOGGLE
};

// â”€â”€â”€ Global State (updated by both local keys and OBS downstream) â”€â”€â”€
static uint8_t  active_camera    = 1;
static bool     is_recording     = false;
static bool     is_streaming     = false;
static int8_t   zoom_level       = 50;   // 0 - 100%
static int8_t   scrub_dir        = 0;    // -1: Rev, 0: Stop, 1: Fwd
static uint8_t  master_vol       = 75;   // 0 - 100%
static uint8_t  mic_gain         = 60;   // 0 - 100%
static uint8_t  replay_speed     = 100;  // 25, 50, 75, 100%
static bool     ch_mute[4]       = {false, false, false, false};
static bool     tally_light_auto = true;
static uint8_t  stream_health    = HEALTH_OK;
static uint8_t  audio_vu[4]      = {0, 0, 0, 0};  // VU peak meters from OBS
static char     oled_custom[8][21];                 // 8 lines Ã— 20 chars from OBS
static bool     oled_custom_active = false;         // When true, OBS controls the OLED

// â”€â”€â”€ Raw HID Downstream Receiver (OBS â†’ Controller) â”€â”€â”€
#ifdef RAW_ENABLE
static bool handle_restrike_raw_hid(uint8_t *data, uint8_t length) {
    uint8_t cmd = data[0];

    switch (cmd) {
        case CMD_SET_ACTIVE_CAMERA:
            // OBS tells us which scene/camera is now LIVE on program output
            active_camera = data[1];
            return true;

        case CMD_SET_REC_STREAM:
            // OBS tells us the real recording and streaming state
            is_recording = (bool)data[1];
            is_streaming = (bool)data[2];
            return true;

        case CMD_SET_AUDIO_LEVELS:
            // Real-time VU peak meters from OBS audio mixer
            audio_vu[0] = data[1];
            audio_vu[1] = data[2];
            audio_vu[2] = data[3];
            audio_vu[3] = data[4];
            return true;

        case CMD_SET_STREAM_HEALTH:
            // 0 = OK (green), 1 = Warning (amber), 2 = Critical (red)
            stream_health = data[1];
            return true;

        case CMD_SET_TALLY_COLOR:
            // Direct per-LED color override from OBS
            #ifdef RGBLIGHT_ENABLE
            {
                uint8_t led_idx = data[1];
                if (led_idx < RESTRIKE_CTR_NUM_LEDS) {
                    rgblight_setrgb_at(data[2], data[3], data[4], led_idx);
                }
            }
            #endif
            return true;

        case CMD_SET_OLED_LINE:
            // OBS pushes custom text to a specific OLED line
            {
                uint8_t line = data[1];
                if (line < 6) {
                    memset(oled_custom[line], 0, 21);
                    memcpy(oled_custom[line], &data[2], 20);
                    oled_custom_active = true;
                }
            }
            return true;

        case CMD_SET_LED_BRIGHTNESS:
            #ifdef RGBLIGHT_ENABLE
            rgblight_sethsv_noeeprom(rgblight_get_hue(), rgblight_get_sat(), data[1]);
            #endif
            return true;

        case CMD_SET_PAGE:
            // OBS commands the controller to switch to a specific page
            if (data[1] < 4) {
                layer_move(data[1]);
            }
            return true;

        case CMD_PING:
            // Host is alive - mark connection as active and reply with PONG + handshake
            obs_connected = true;
            oled_custom_active = false;  // Reset custom OLED on reconnect
            memset(oled_custom, 0, sizeof(oled_custom));
            {
                uint8_t pong_data[RAW_EPSIZE];
                memset(pong_data, 0, RAW_EPSIZE);
                pong_data[0] = CMD_PONG;
                raw_hid_send(pong_data, RAW_EPSIZE);
            }
            restrike_send_handshake();
            return true;

        default:
            return false;
    }
}

#if defined(VIA_ENABLE)
bool via_command_kb(uint8_t *data, uint8_t length) {
    if (handle_restrike_raw_hid(data, length)) {
        return true;
    }
    return false;
}
#else
void raw_hid_receive(uint8_t *data, uint8_t length) {
    handle_restrike_raw_hid(data, length);
}
#endif

#endif // RAW_ENABLE

// â”€â”€â”€ Key Matrix Index Lookup (for upstream event reporting) â”€â”€â”€
// Maps custom keycodes to a sequential key index 0..9
static uint8_t keycode_to_key_idx(uint16_t keycode) {
    switch (keycode) {
        case CAM_1:         return 0; // [0,0] Top-Left
        case CAM_2:         return 1; // [0,1] Top-Mid
        case KC_F17:        return 2; // [0,2] Top-Right 1
        case KC_F18:        return 3; // [1,2] Top-Right 2
        case CAM_3:         return 4; // [1,0] 2nd Row Left
        case CAM_4:         return 5; // [1,1] 2nd Row Mid
        case KC_MPLY:       return 6; // [0,3] Play/Pause
        case REC_TOGGLE:    return 7; // [1,3] Start/Stop
        case PAGE_CYCLE:    return 8; // [0,4] Zoom Click
        case KC_MUTE:       return 9; // [1,4] Shuffle Click
        default:            return 0xFF;
    }
}

// â”€â”€â”€ Keymaps for 4 Pages â”€â”€â”€
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_PAGE_BROADCAST] = LAYOUT(
        CAM_1,       CAM_2,       KC_F17,      KC_F18,
        CAM_3,       CAM_4,
        KC_MPLY,     REC_TOGGLE,
        PAGE_CYCLE,  KC_MUTE
    ),

    [_PAGE_AUDIO] = LAYOUT(
        AUD_MUTE_CH1, AUD_MUTE_CH2, KC_F20,       KC_F21,
        AUD_MUTE_CH3, AUD_MUTE_CH4,
        KC_MUTE,      KC_F22,
        PAGE_CYCLE,   KC_F23
    ),

    [_PAGE_REPLAY] = LAYOUT(
        SPEED_25,    SPEED_50,    MARK_IN,     MARK_OUT,
        SPEED_75,    SPEED_100,
        KC_SPACE,    SAVE_REPLAY,
        PAGE_CYCLE,  KC_HOME
    ),

    [_PAGE_LIGHTING] = LAYOUT(
        UG_TOGG,     UG_NEXT,     UG_HUEU,     UG_HUED,
        UG_PREV,     RGB_TALLY_TOGGLE,
        UG_SATU,     UG_SATD,
        PAGE_CYCLE,  RGB_M_P
    )
};

// â”€â”€â”€ Key Event Processor â”€â”€â”€
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Always send upstream Raw HID event for every physical key press/release
    #ifdef RAW_ENABLE
    uint8_t idx = matrix_to_key_idx(record->event.key.row, record->event.key.col);
    if (idx != 0xFF) {
        restrike_send_key_event(idx, record->event.pressed);
    }
    #endif

    if (record->event.pressed) {
        switch (keycode) {
            // Page Cycling (Zoom Knob Click)
            case PAGE_CYCLE: {
                uint8_t current_page = get_highest_layer(layer_state);
                uint8_t next_page = (current_page + 1) % 4;
                layer_move(next_page);
                return false;
            }

            case PAGE_PREV: {
                uint8_t current_page = get_highest_layer(layer_state);
                uint8_t prev_page = (current_page == 0) ? 3 : (current_page - 1);
                layer_move(prev_page);
                return false;
            }

            // Camera Selection (Page 1)
            case CAM_1:
                active_camera = 1;
                tap_code16(KC_F13);
                return false;
            case CAM_2:
                active_camera = 2;
                tap_code16(KC_F14);
                return false;
            case CAM_3:
                active_camera = 3;
                tap_code16(KC_F15);
                return false;
            case CAM_4:
                active_camera = 4;
                tap_code16(KC_F16);
                return false;

            // Broadcast Toggles
            case REC_TOGGLE:
                is_recording = !is_recording;
                tap_code16(KC_F19);
                return false;
            case STREAM_TOGGLE:
                is_streaming = !is_streaming;
                tap_code16(C(KC_F19));
                return false;

            // Audio Mutes (Page 2)
            case AUD_MUTE_CH1:
                ch_mute[0] = !ch_mute[0];
                tap_code16(C(A(KC_1)));
                return false;
            case AUD_MUTE_CH2:
                ch_mute[1] = !ch_mute[1];
                tap_code16(C(A(KC_2)));
                return false;
            case AUD_MUTE_CH3:
                ch_mute[2] = !ch_mute[2];
                tap_code16(C(A(KC_3)));
                return false;
            case AUD_MUTE_CH4:
                ch_mute[3] = !ch_mute[3];
                tap_code16(C(A(KC_4)));
                return false;

            // Replay Controls (Page 3)
            case SPEED_25:  replay_speed = 25;  tap_code16(C(KC_1)); return false;
            case SPEED_50:  replay_speed = 50;  tap_code16(C(KC_2)); return false;
            case SPEED_75:  replay_speed = 75;  tap_code16(C(KC_3)); return false;
            case SPEED_100: replay_speed = 100; tap_code16(C(KC_4)); return false;
            case MARK_IN:   tap_code16(KC_LBRC); return false;
            case MARK_OUT:  tap_code16(KC_RBRC); return false;
            case SAVE_REPLAY: tap_code16(KC_F24); return false;

            // Lighting Controls (Page 4)
            case RGB_TALLY_TOGGLE:
                tally_light_auto = !tally_light_auto;
                return false;

            default:
                break;
        }
    }
    return true;
}

// â”€â”€â”€ Context-Aware Rotary Encoders â”€â”€â”€
#if !defined(ENCODER_MAP_ENABLE)
bool encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t current_page = get_highest_layer(layer_state);

    // Always send upstream encoder event when OBS is connected
    #ifdef RAW_ENABLE
    if (obs_connected) {
        restrike_send_encoder_event(index, clockwise ? 1 : -1, 1);
    }
    #endif

    if (index == 0) {
        switch (current_page) {
            case _PAGE_BROADCAST:
                if (clockwise) {
                    if (zoom_level < 100) zoom_level += 5;
                    tap_code16(C(KC_EQL));
                } else {
                    if (zoom_level > 0) zoom_level -= 5;
                    tap_code16(C(KC_MINS));
                }
                break;
            case _PAGE_AUDIO:
                if (clockwise) {
                    if (mic_gain < 100) mic_gain += 5;
                    tap_code16(C(KC_VOLU));
                } else {
                    if (mic_gain > 0) mic_gain -= 5;
                    tap_code16(C(KC_VOLD));
                }
                break;
            case _PAGE_REPLAY:
                if (clockwise) tap_code16(C(KC_RGHT));
                else           tap_code16(C(KC_LEFT));
                break;
            case _PAGE_LIGHTING:
                if (clockwise) rgblight_increase_val();
                else           rgblight_decrease_val();
                break;
        }
    } else if (index == 1) {
        switch (current_page) {
            case _PAGE_BROADCAST:
                scrub_dir = clockwise ? 1 : -1;
                tap_code16(clockwise ? KC_RGHT : KC_LEFT);
                break;
            case _PAGE_AUDIO:
                if (clockwise) {
                    if (master_vol < 100) master_vol += 2;
                    tap_code16(KC_VOLU);
                } else {
                    if (master_vol > 0) master_vol -= 2;
                    tap_code16(KC_VOLD);
                }
                break;
            case _PAGE_REPLAY:
                scrub_dir = clockwise ? 1 : -1;
                tap_code16(clockwise ? KC_RGHT : KC_LEFT);
                break;
            case _PAGE_LIGHTING:
                if (clockwise) rgblight_increase_hue();
                else           rgblight_decrease_hue();
                break;
        }
    }
    return false;
}
#endif

void keyboard_post_init_user(void) {
#if defined(VIA_ENABLE)
    dynamic_keymap_reset();
#endif
}

// â”€â”€â”€ OLED HUD Display with OBS Sync â”€â”€â”€
#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    oled_clear();
    return OLED_ROTATION_0;
}

bool oled_task_user(void) {
    // If OBS has pushed custom OLED content (up to 8 lines), render it
    if (oled_custom_active) {
        for (uint8_t i = 0; i < 8; i++) {
            oled_set_cursor(0, i);
            if (oled_custom[i][0] != '\0') {
                oled_write(oled_custom[i], false);
            }
        }
        return false;
    }

    uint8_t current_page = get_highest_layer(layer_state);

    switch (current_page) {
        // â”€â”€â”€ PAGE 1: BROADCAST HUD â”€â”€â”€
        case _PAGE_BROADCAST: {
            oled_set_cursor(0, 0);
            oled_write_P(PSTR("  RE-STRIKE STUDIO  "), false);

            oled_set_cursor(0, 2);
            oled_write_P(PSTR("   CAMERA: [ "), false);
            char cam_str[3];
            itoa(active_camera, cam_str, 10);
            oled_write(cam_str, false);
            oled_write_P(PSTR(" ]     "), false);

            oled_set_cursor(0, 3);
            if (is_recording && is_streaming) {
                oled_write_P(PSTR("  STATUS: REC+LIVE  "), true);
            } else if (is_recording) {
                oled_write_P(PSTR("   STATUS: *REC*    "), true);
            } else if (is_streaming) {
                oled_write_P(PSTR("   STATUS: LIVE     "), false);
            } else {
                oled_write_P(PSTR("   STATUS: STBY     "), false);
            }

            oled_set_cursor(0, 4);
            oled_write_P(PSTR("   ZOOM  : "), false);
            char zm_str[4];
            itoa(zoom_level, zm_str, 10);
            if (zoom_level < 10) oled_write_P(PSTR("  "), false);
            else if (zoom_level < 100) oled_write_P(PSTR(" "), false);
            oled_write(zm_str, false);
            oled_write_P(PSTR("%     "), false);

            oled_set_cursor(0, 5);
            if (scrub_dir > 0) {
                oled_write_P(PSTR("   JOG   : FWD 2x   "), false);
            } else if (scrub_dir < 0) {
                oled_write_P(PSTR("   JOG   : REV 2x   "), false);
            } else {
                oled_write_P(PSTR("   JOG   : IDLE     "), false);
            }

            oled_set_cursor(0, 7);
            oled_write_P(PSTR(" E1: ZOOM   E2: JOG "), false);
            break;
        }

        // â”€â”€â”€ PAGE 2: AUDIO MIXER â”€â”€â”€
        case _PAGE_AUDIO: {
            oled_set_cursor(0, 0);
            oled_write_P(PSTR("    AUDIO MIXER     "), false);

            oled_set_cursor(0, 2);
            oled_write_P(PSTR("   INPUTS: 1-4 ON   "), false);

            oled_set_cursor(0, 3);
            oled_write_P(PSTR("   MASTER: "), false);
            char v_str[4];
            itoa(master_vol, v_str, 10);
            if (master_vol < 10) oled_write_P(PSTR("  "), false);
            else if (master_vol < 100) oled_write_P(PSTR(" "), false);
            oled_write(v_str, false);
            oled_write_P(PSTR("%     "), false);

            oled_set_cursor(0, 4);
            oled_write_P(PSTR("   MIC   : "), false);
            char g_str[4];
            itoa(mic_gain, g_str, 10);
            if (mic_gain < 10) oled_write_P(PSTR("  "), false);
            else if (mic_gain < 100) oled_write_P(PSTR(" "), false);
            oled_write(g_str, false);
            oled_write_P(PSTR("%     "), false);

            oled_set_cursor(0, 5);
            oled_write_P(PSTR("   PEAK  : -6 dB    "), false);

            oled_set_cursor(0, 7);
            oled_write_P(PSTR(" E1: VOL    E2: GAIN"), false);
            break;
        }

        // â”€â”€â”€ PAGE 3: INSTANT REPLAY â”€â”€â”€
        case _PAGE_REPLAY: {
            oled_set_cursor(0, 0);
            oled_write_P(PSTR("   INSTANT REPLAY   "), false);

            oled_set_cursor(0, 2);
            oled_write_P(PSTR("   SPEED : "), false);
            char spd_str[4];
            itoa(replay_speed, spd_str, 10);
            if (replay_speed < 10) oled_write_P(PSTR("  "), false);
            else if (replay_speed < 100) oled_write_P(PSTR(" "), false);
            oled_write(spd_str, false);
            oled_write_P(PSTR("%     "), false);

            oled_set_cursor(0, 3);
            oled_write_P(PSTR("   BUFFER: READY    "), false);

            oled_set_cursor(0, 4);
            oled_write_P(PSTR("   MARK  : SAVED    "), false);

            oled_set_cursor(0, 5);
            oled_write_P(PSTR("   CLIP  : 00:06s   "), false);

            oled_set_cursor(0, 7);
            oled_write_P(PSTR(" E1: SPEED  E2: SHUT"), false);
            break;
        }

        // â”€â”€â”€ PAGE 4: LIGHTING & RGB â”€â”€â”€
        case _PAGE_LIGHTING: {
            oled_set_cursor(0, 0);
            oled_write_P(PSTR("    SYSTEM SETUP    "), false);

            oled_set_cursor(0, 2);
            oled_write_P(PSTR("   LIGHTS: "), false);
            oled_write_P(rgblight_is_enabled() ? PSTR("ON       ") : PSTR("OFF      "), false);

            oled_set_cursor(0, 3);
            oled_write_P(PSTR("   TALLY : "), false);
            oled_write_P(tally_light_auto ? PSTR("AUTO     ") : PSTR("MANUAL   "), false);

            oled_set_cursor(0, 4);
            oled_write_P(PSTR("   BRIGHT: "), false);
            uint8_t brt = (rgblight_get_val() * 100) / 255;
            char b_str[4];
            itoa(brt, b_str, 10);
            if (brt < 10) oled_write_P(PSTR("  "), false);
            else if (brt < 100) oled_write_P(PSTR(" "), false);
            oled_write(b_str, false);
            oled_write_P(PSTR("%     "), false);

            oled_set_cursor(0, 5);
            oled_write_P(PSTR("   STATUS: READY    "), false);

            oled_set_cursor(0, 7);
            oled_write_P(PSTR(" E1: BRT    E2: HUE "), false);
            break;
        }
    }

    return false;
}
#endif

// â”€â”€â”€ Reactive ARGB Tally Lighting Engine â”€â”€â”€
void housekeeping_task_user(void) {
    #if defined(RGBLIGHT_ENABLE)
    if (tally_light_auto && get_highest_layer(layer_state) == _PAGE_BROADCAST) {
        uint8_t cam_led_idx = active_camera - 1;

        // Set active camera LED based on recording state
        if (is_recording) {
            rgblight_sethsv_at(0, 255, 200, cam_led_idx);    // RED = Program/Recording
        } else {
            rgblight_sethsv_at(85, 255, 200, cam_led_idx);   // GREEN = Preview/Standby
        }

        // Status LEDs 8-11: Stream health indicator
        if (obs_connected) {
            switch (stream_health) {
                case HEALTH_OK:
                    rgblight_sethsv_at(85, 255, 150, 9);     // Green
                    break;
                case HEALTH_WARNING:
                    rgblight_sethsv_at(30, 255, 200, 9);     // Amber
                    break;
                case HEALTH_CRITICAL:
                    rgblight_sethsv_at(0, 255, 255, 9);      // Red pulse
                    break;
            }

            // LED 10: Recording tally
            if (is_recording) {
                rgblight_sethsv_at(0, 255, 200, 10);         // Red
            } else {
                rgblight_sethsv_at(0, 0, 30, 10);            // Dim white
            }

            // LED 11: Streaming tally
            if (is_streaming) {
                rgblight_sethsv_at(170, 255, 200, 11);       // Blue
            } else {
                rgblight_sethsv_at(0, 0, 30, 11);            // Dim white
            }
        }
    }
    #endif
}


