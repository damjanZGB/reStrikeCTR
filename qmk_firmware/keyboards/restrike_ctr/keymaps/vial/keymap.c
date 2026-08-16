#include QMK_KEYBOARD_H
#include "restrike_protocol.h"

#ifdef RAW_ENABLE
#include "raw_hid.h"
#endif

// ─── External symbols from restrike_ctr.c ───
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

// ─── Global State (updated by both local keys and OBS downstream) ───
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
static char     oled_custom[6][21];                 // 6 lines × 20 chars from OBS
static bool     oled_custom_active = false;         // When true, OBS controls the OLED

// ─── Raw HID Downstream Receiver (OBS → Controller) ───
#ifdef RAW_ENABLE
void raw_hid_receive(uint8_t *data, uint8_t length) {
    uint8_t cmd = data[0];

    switch (cmd) {
        case CMD_SET_ACTIVE_CAMERA:
            // OBS tells us which scene/camera is now LIVE on program output
            active_camera = data[1];
            break;

        case CMD_SET_REC_STREAM:
            // OBS tells us the real recording and streaming state
            is_recording = (bool)data[1];
            is_streaming = (bool)data[2];
            break;

        case CMD_SET_AUDIO_LEVELS:
            // Real-time VU peak meters from OBS audio mixer
            audio_vu[0] = data[1];
            audio_vu[1] = data[2];
            audio_vu[2] = data[3];
            audio_vu[3] = data[4];
            break;

        case CMD_SET_STREAM_HEALTH:
            // 0 = OK (green), 1 = Warning (amber), 2 = Critical (red)
            stream_health = data[1];
            break;

        case CMD_SET_TALLY_COLOR:
            // Direct per-LED color override from OBS
            #ifdef RGBLIGHT_ENABLE
            {
                uint8_t led_idx = data[1];
                if (led_idx < RGBLED_NUM) {
                    rgblight_setrgb_at(data[2], data[3], data[4], led_idx);
                }
            }
            #endif
            break;

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
            break;

        case CMD_SET_LED_BRIGHTNESS:
            #ifdef RGBLIGHT_ENABLE
            rgblight_sethsv_noeeprom(rgblight_get_hue(), rgblight_get_sat(), data[1]);
            #endif
            break;

        case CMD_SET_PAGE:
            // OBS commands the controller to switch to a specific page
            if (data[1] < 4) {
                layer_move(data[1]);
            }
            break;

        case CMD_PING:
            // Host is alive - mark connection as active and reply with PONG + handshake
            obs_connected = true;
            oled_custom_active = false;  // Reset custom OLED on reconnect
            memset(oled_custom, 0, sizeof(oled_custom));
            {
                uint8_t pong[1] = {0};
                uint8_t pong_data[RAW_EPSIZE];
                memset(pong_data, 0, RAW_EPSIZE);
                pong_data[0] = CMD_PONG;
                raw_hid_send(pong_data, RAW_EPSIZE);
            }
            restrike_send_handshake();
            break;

        default:
            break;
    }
}
#endif // RAW_ENABLE

// ─── Key Matrix Index Lookup (for upstream event reporting) ───
// Maps custom keycodes to a sequential key index 0..9
static uint8_t keycode_to_key_idx(uint16_t keycode) {
    switch (keycode) {
        case CAM_1:         return 0;
        case CAM_2:         return 1;
        case CAM_3:         return 2;
        case CAM_4:         return 3;
        case KC_F17:        return 4;  // AUX 1
        case KC_F18:        return 5;  // AUX 2
        case KC_MPLY:       return 6;  // Play/Pause
        case REC_TOGGLE:    return 7;  // Start/Stop
        case PAGE_CYCLE:    return 8;  // Zoom Click
        case KC_MUTE:       return 9;  // Shuffle Click
        default:            return 0xFF;
    }
}

// ─── Keymaps for 4 Pages ───
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_PAGE_BROADCAST] = LAYOUT(
        CAM_1,       CAM_2,       CAM_3,       CAM_4,
        KC_F17,      KC_F18,
        KC_MPLY,     REC_TOGGLE,
        PAGE_CYCLE,  KC_MUTE
    ),

    [_PAGE_AUDIO] = LAYOUT(
        AUD_MUTE_CH1, AUD_MUTE_CH2, AUD_MUTE_CH3, AUD_MUTE_CH4,
        KC_F20,       KC_F21,
        KC_MUTE,      KC_F22,
        PAGE_CYCLE,   KC_F23
    ),

    [_PAGE_REPLAY] = LAYOUT(
        SPEED_25,    SPEED_50,    SPEED_75,    SPEED_100,
        MARK_IN,     MARK_OUT,
        KC_SPACE,    SAVE_REPLAY,
        PAGE_CYCLE,  KC_HOME
    ),

    [_PAGE_LIGHTING] = LAYOUT(
        RGB_TOG,     RGB_MOD,     RGB_RMOD,    RGB_TALLY_TOGGLE,
        RGB_HUI,     RGB_HUD,
        RGB_SAI,     RGB_SAD,
        PAGE_CYCLE,  RGB_M_P
    )
};

// ─── Key Event Processor ───
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    // Send upstream HID event for every key press/release when OBS is connected
    #ifdef RAW_ENABLE
    if (obs_connected) {
        uint8_t idx = keycode_to_key_idx(keycode);
        if (idx != 0xFF) {
            restrike_send_key_event(idx, record->event.pressed);
        }
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

// ─── Context-Aware Rotary Encoders ───
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

// ─── OLED HUD Display with OBS Sync ───
#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_0;
}

// Render a horizontal VU meter bar: "label [####......] val"
static void render_vu_bar(const char *label, uint8_t value) {
    oled_write_P(label, false);
    oled_write_P(PSTR("["), false);
    uint8_t bars = value / 10;
    for (uint8_t i = 0; i < 10; i++) {
        oled_write_P((i < bars) ? PSTR("#") : PSTR("."), false);
    }
    oled_write_P(PSTR("]\n"), false);
}

bool oled_task_user(void) {
    // If OBS has pushed custom OLED content, render it instead of local HUD
    if (oled_custom_active) {
        for (uint8_t i = 0; i < 6; i++) {
            if (oled_custom[i][0] != '\0') {
                oled_write(oled_custom[i], false);
                oled_write_P(PSTR("\n"), false);
            }
        }
        return false;
    }

    uint8_t current_page = get_highest_layer(layer_state);

    // Connection status indicator
    oled_write_P(obs_connected ? PSTR("*") : PSTR(" "), false);

    switch (current_page) {
        case _PAGE_BROADCAST: {
            oled_write_P(PSTR(" BROADCAST [P1/4]\n"), false);

            oled_write_P(PSTR("CAM: [ "), false);
            char cam_str[3];
            itoa(active_camera, cam_str, 10);
            oled_write(cam_str, false);
            oled_write_P(PSTR(" ] "), false);

            if (is_recording && is_streaming) {
                oled_write_P(PSTR("REC+LIVE\n"), true);
            } else if (is_recording) {
                oled_write_P(PSTR("*REC*\n"), true);
            } else if (is_streaming) {
                oled_write_P(PSTR("LIVE\n"), false);
            } else {
                oled_write_P(PSTR("STBY\n"), false);
            }

            render_vu_bar(PSTR("ZOOM: "), zoom_level);

            oled_write_P(PSTR("JOG:  "), false);
            if (scrub_dir > 0) oled_write_P(PSTR(">> FWD  "), false);
            else if (scrub_dir < 0) oled_write_P(PSTR("<< REV  "), false);
            else oled_write_P(PSTR("-- IDLE "), false);

            // Stream health indicator
            switch (stream_health) {
                case HEALTH_WARNING:  oled_write_P(PSTR("!\n"), true); break;
                case HEALTH_CRITICAL: oled_write_P(PSTR("X\n"), true); break;
                default:              oled_write_P(PSTR("\n"), false); break;
            }
            break;
        }

        case _PAGE_AUDIO: {
            oled_write_P(PSTR(" AUDIO MIX [P2/4]\n"), false);

            // Channel mute states
            for (uint8_t i = 0; i < 4; i++) {
                oled_write_P(PSTR("C"), false);
                char ch_str[2];
                itoa(i + 1, ch_str, 10);
                oled_write(ch_str, false);
                oled_write_P(PSTR(":"), false);
                oled_write_P(ch_mute[i] ? PSTR("MUT ") : PSTR("ON  "), false);
                if (i == 1 || i == 3) oled_write_P(PSTR("\n"), false);
            }

            // If OBS is sending live VU meters, show them
            if (obs_connected && (audio_vu[0] > 0 || audio_vu[1] > 0)) {
                render_vu_bar(PSTR("VU-1: "), audio_vu[0]);
                render_vu_bar(PSTR("VU-2: "), audio_vu[1]);
            } else {
                render_vu_bar(PSTR("M-VOL:"), master_vol);
                render_vu_bar(PSTR("MIC:  "), mic_gain);
            }
            break;
        }

        case _PAGE_REPLAY: {
            oled_write_P(PSTR(" REPLAY UI [P3/4]\n"), false);

            oled_write_P(PSTR("SPEED: "), false);
            char spd_str[4];
            itoa(replay_speed, spd_str, 10);
            oled_write(spd_str, false);
            oled_write_P(PSTR("%\n"), false);

            oled_write_P(PSTR("CLIP:  [ READY ]\n"), false);
            oled_write_P(PSTR("MARK: IN:[OK] OUT:[OK]\n"), false);
            oled_write_P(PSTR("SHUTTLE: FRAME SCRUB\n"), false);
            break;
        }

        case _PAGE_LIGHTING: {
            oled_write_P(PSTR(" RGB SETUP [P4/4]\n"), false);

            oled_write_P(PSTR("RGB: "), false);
            oled_write_P(rgblight_is_enabled() ? PSTR("ON  ") : PSTR("OFF "), false);
            oled_write_P(PSTR("TALLY: "), false);
            oled_write_P(tally_light_auto ? PSTR("AUTO\n") : PSTR("MAN\n"), false);

            oled_write_P(PSTR("HUE:"), false);
            char h_str[4]; itoa(rgblight_get_hue(), h_str, 10);
            oled_write(h_str, false);
            oled_write_P(PSTR(" SAT:"), false);
            char s_str[4]; itoa(rgblight_get_sat(), s_str, 10);
            oled_write(s_str, false);
            oled_write_P(PSTR("\n"), false);

            render_vu_bar(PSTR("BRT:  "), (rgblight_get_val() * 100) / 255);
            oled_write_P(PSTR("ENC1:BRT  ENC2:HUE\n"), false);
            break;
        }
    }

    return false;
}
#endif

// ─── Reactive ARGB Tally Lighting Engine ───
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
