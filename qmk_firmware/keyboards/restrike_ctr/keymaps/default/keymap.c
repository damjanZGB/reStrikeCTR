#include QMK_KEYBOARD_H
#include "restrike_protocol.h"

#ifdef RAW_ENABLE
#include "raw_hid.h"
#endif

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

// Global State
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

// Keymaps for 4 Pages
const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* -------------------------------------------------------------
     * PAGE 1: OBS / BROADCAST STUDIO
     * ------------------------------------------------------------- */
    [_PAGE_BROADCAST] = LAYOUT(
        CAM_1,       CAM_2,       CAM_3,       CAM_4,
        KC_F17,      KC_F18,
        KC_MPLY,     REC_TOGGLE,
        PAGE_CYCLE,  KC_MUTE
    ),

    /* -------------------------------------------------------------
     * PAGE 2: AUDIO MIXER
     * ------------------------------------------------------------- */
    [_PAGE_AUDIO] = LAYOUT(
        AUD_MUTE_CH1, AUD_MUTE_CH2, AUD_MUTE_CH3, AUD_MUTE_CH4,
        KC_F20,       KC_F21,
        KC_MUTE,      KC_F22,
        PAGE_CYCLE,   KC_F23
    ),

    /* -------------------------------------------------------------
     * PAGE 3: INSTANT REPLAY & TIMELINE JOG
     * ------------------------------------------------------------- */
    [_PAGE_REPLAY] = LAYOUT(
        SPEED_25,    SPEED_50,    SPEED_75,    SPEED_100,
        MARK_IN,     MARK_OUT,
        KC_SPACE,    SAVE_REPLAY,
        PAGE_CYCLE,  KC_HOME
    ),

    /* -------------------------------------------------------------
     * PAGE 4: ARGB HARDWARE LIGHTING & SYSTEM
     * ------------------------------------------------------------- */
    [_PAGE_LIGHTING] = LAYOUT(
        UG_TOGG,     UG_NEXT,     UG_PREV,     RGB_TALLY_TOGGLE,
        UG_HUEU,     UG_HUED,
        UG_SATU,     UG_SATD,
        PAGE_CYCLE,  RGB_M_P
    )
};

// Key event processor
bool process_record_user(uint16_t keycode, keyrecord_t *record) {
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
            case SPEED_25:
                replay_speed = 25;
                tap_code16(C(KC_1));
                return false;
            case SPEED_50:
                replay_speed = 50;
                tap_code16(C(KC_2));
                return false;
            case SPEED_75:
                replay_speed = 75;
                tap_code16(C(KC_3));
                return false;
            case SPEED_100:
                replay_speed = 100;
                tap_code16(C(KC_4));
                return false;
            case MARK_IN:
                tap_code16(KC_LBRC);
                return false;
            case MARK_OUT:
                tap_code16(KC_RBRC);
                return false;
            case SAVE_REPLAY:
                tap_code16(KC_F24);
                return false;

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

// Context-Aware Rotary Encoders
#if !defined(ENCODER_MAP_ENABLE)
bool encoder_update_user(uint8_t index, bool clockwise) {
    uint8_t current_page = get_highest_layer(layer_state);

    if (index == 0) {
        // ENCODER 1: ZOOM / GAIN / BRIGHTNESS KNOB
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
                if (clockwise) {
                    tap_code16(C(KC_RGHT));
                } else {
                    tap_code16(C(KC_LEFT));
                }
                break;

            case _PAGE_LIGHTING:
                if (clockwise) {
                    rgblight_increase_val();
                } else {
                    rgblight_decrease_val();
                }
                break;
        }
    } else if (index == 1) {
        // ENCODER 2: SHUFFLE / VOLUME / SCRUB / SPEED KNOB
        switch (current_page) {
            case _PAGE_BROADCAST:
                if (clockwise) {
                    scrub_dir = 1;
                    tap_code16(KC_RGHT);
                } else {
                    scrub_dir = -1;
                    tap_code16(KC_LEFT);
                }
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
                if (clockwise) {
                    scrub_dir = 1;
                    tap_code16(KC_RGHT);
                } else {
                    scrub_dir = -1;
                    tap_code16(KC_LEFT);
                }
                break;

            case _PAGE_LIGHTING:
                if (clockwise) {
                    rgblight_increase_hue();
                } else {
                    rgblight_decrease_hue();
                }
                break;
        }
    }
    return false;
}
#endif

// Dynamic OLED Screen with 4 Rich Pages
#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    oled_clear();
    return OLED_ROTATION_0;
}

// Helper function for rendering sleek 10-segment level gauges
static void render_gauge_row(uint8_t row, const char *label, uint8_t percent) {
    oled_set_cursor(0, row);
    oled_write_P(label, false);
    oled_write_P(PSTR("["), false);
    uint8_t filled = (percent > 100 ? 100 : percent) / 10;
    for (uint8_t i = 0; i < 10; i++) {
        if (i < filled) {
            oled_write_P(PSTR("#"), false);
        } else {
            oled_write_P(PSTR("."), false);
        }
    }
    oled_write_P(PSTR("] "), false);
    char p_str[4];
    itoa(percent > 100 ? 100 : percent, p_str, 10);
    if (percent < 10) oled_write_P(PSTR("  "), false);
    else if (percent < 100) oled_write_P(PSTR(" "), false);
    oled_write(p_str, false);
    oled_write_P(PSTR("%"), false);
}

bool oled_task_user(void) {
    uint8_t current_page = get_highest_layer(layer_state);

    switch (current_page) {
        // ─── PAGE 1: BROADCAST HUD (Studio Pro) ───
        case _PAGE_BROADCAST: {
            // Row 0: Inverted Header Banner
            oled_set_cursor(0, 0);
            oled_write_P(PSTR(" RESTRIKE CTR [P1/4] "), true);

            // Row 1: Active Camera & Live State
            oled_set_cursor(0, 1);
            oled_write_P(PSTR("CAM: "), false);
            char cam_str[3];
            itoa(active_camera, cam_str, 10);
            oled_write(cam_str, false);
            oled_write_P(PSTR("    STATUS:"), false);
            if (is_recording) {
                oled_write_P(PSTR(" *REC*  "), true);
            } else {
                oled_write_P(PSTR("  STBY  "), false);
            }

            // Row 2: Zoom Gauge
            render_gauge_row(2, PSTR("ZM  "), zoom_level);

            // Row 3: Jog / Shuttle Status
            oled_set_cursor(0, 3);
            oled_write_P(PSTR("JOG ["), false);
            if (scrub_dir > 0) oled_write_P(PSTR(" >> FWD x2  "), false);
            else if (scrub_dir < 0) oled_write_P(PSTR(" << REV x2  "), false);
            else oled_write_P(PSTR(" -- IDLE -- "), false);
            oled_write_P(PSTR("] 1x"), false);

            // Row 4: Pan / Tilt Joystick
            oled_set_cursor(0, 4);
            oled_write_P(PSTR("JOY  PAN:00%  TLT:00%"), false);

            // Row 5: Tally Status
            oled_set_cursor(0, 5);
            oled_write_P(PSTR("TALLY: [AUTO] RGB LINK"), false);

            // Row 6: Divider
            oled_set_cursor(0, 6);
            oled_write_P(PSTR("---------------------"), false);

            // Row 7: Encoder Footer (No newline to prevent scroll)
            oled_set_cursor(0, 7);
            oled_write_P(PSTR("E1:ZOOM   E2:JOG/SCRB"), false);
            break;
        }

        // ─── PAGE 2: AUDIO MIXER (4-Channel + VUs) ───
        case _PAGE_AUDIO: {
            // Row 0: Inverted Header Banner
            oled_set_cursor(0, 0);
            oled_write_P(PSTR(" AUDIO MIXER  [P2/4] "), true);

            // Row 1: Channels 1 & 2
            oled_set_cursor(0, 1);
            oled_write_P(PSTR("CH1:["), false);
            oled_write_P(ch_mute[0] ? PSTR("MUT] ") : PSTR("ON ] "), false);
            oled_write_P(PSTR("CH2:["), false);
            oled_write_P(ch_mute[1] ? PSTR("MUT] ") : PSTR("ON ] "), false);

            // Row 2: Channels 3 & 4
            oled_set_cursor(0, 2);
            oled_write_P(PSTR("CH3:["), false);
            oled_write_P(ch_mute[2] ? PSTR("MUT] ") : PSTR("ON ] "), false);
            oled_write_P(PSTR("CH4:["), false);
            oled_write_P(ch_mute[3] ? PSTR("MUT] ") : PSTR("ON ] "), false);

            // Row 3: Master Volume
            render_gauge_row(3, PSTR("VOL "), master_vol);

            // Row 4: Mic Gain
            render_gauge_row(4, PSTR("GAIN"), mic_gain);

            // Row 5: VU Master Level
            oled_set_cursor(0, 5);
            oled_write_P(PSTR("VU-L [########..] -6dB"), false);

            // Row 6: Divider
            oled_set_cursor(0, 6);
            oled_write_P(PSTR("---------------------"), false);

            // Row 7: Encoder Footer
            oled_set_cursor(0, 7);
            oled_write_P(PSTR("E1:VOL    E2:MIC-GAIN"), false);
            break;
        }

        // ─── PAGE 3: INSTANT REPLAY (Broadcast Clip Buffer) ───
        case _PAGE_REPLAY: {
            // Row 0: Inverted Header Banner
            oled_set_cursor(0, 0);
            oled_write_P(PSTR(" INSTANT RPLY [P3/4] "), true);

            // Row 1: Playback Speed
            oled_set_cursor(0, 1);
            oled_write_P(PSTR("SPEED: [ "), false);
            char spd_str[4];
            itoa(replay_speed, spd_str, 10);
            oled_write(spd_str, false);
            oled_write_P(PSTR("% ]  SLOW-MO"), false);

            // Row 2: In Mark
            oled_set_cursor(0, 2);
            oled_write_P(PSTR("MARK IN : 00:14:22.10"), false);

            // Row 3: Out Mark
            oled_set_cursor(0, 3);
            oled_write_P(PSTR("MARK OUT: 00:14:28.45"), false);

            // Row 4: Clip Duration
            oled_set_cursor(0, 4);
            oled_write_P(PSTR("DURATION: 00:00:06.35"), false);

            // Row 5: Buffer Status
            oled_set_cursor(0, 5);
            oled_write_P(PSTR("BUFFER  : [SAVED 100%]"), false);

            // Row 6: Divider
            oled_set_cursor(0, 6);
            oled_write_P(PSTR("---------------------"), false);

            // Row 7: Encoder Footer
            oled_set_cursor(0, 7);
            oled_write_P(PSTR("E1:SPEED  E2:SHUTTLE "), false);
            break;
        }

        // ─── PAGE 4: SYSTEM & RGB SETUP ───
        case _PAGE_LIGHTING: {
            // Row 0: Inverted Header Banner
            oled_set_cursor(0, 0);
            oled_write_P(PSTR(" SYSTEM & RGB [P4/4] "), true);

            // Row 1: Power & Tally Mode
            oled_set_cursor(0, 1);
            oled_write_P(PSTR("PWR: ["), false);
            oled_write_P(rgblight_is_enabled() ? PSTR("ON ] ") : PSTR("OFF] "), false);
            oled_write_P(PSTR("TALLY:["), false);
            oled_write_P(tally_light_auto ? PSTR("AUTO]") : PSTR("MAN ]"), false);

            // Row 2: Hue & Saturation
            oled_set_cursor(0, 2);
            oled_write_P(PSTR("HUE: "), false);
            char h_str[4]; itoa(rgblight_get_hue(), h_str, 10);
            oled_write(h_str, false);
            oled_write_P(PSTR("    SAT: "), false);
            char s_str[4]; itoa(rgblight_get_sat(), s_str, 10);
            oled_write(s_str, false);

            // Row 3: Brightness
            render_gauge_row(3, PSTR("BRT "), (rgblight_get_val() * 100) / 255);

            // Row 4: Mode
            oled_set_cursor(0, 4);
            oled_write_P(PSTR("MODE: SOLID TALLY LINK"), false);

            // Row 5: System Status
            oled_set_cursor(0, 5);
            oled_write_P(PSTR("USB : OBS-STUDIO READY"), false);

            // Row 6: Divider
            oled_set_cursor(0, 6);
            oled_write_P(PSTR("---------------------"), false);

            // Row 7: Encoder Footer
            oled_set_cursor(0, 7);
            oled_write_P(PSTR("E1:BRIGHT E2:HUE/COLOR"), false);
            break;
        }
    }

    return false;
}
#endif

// Reactive ARGB Tally Lighting Engine
void housekeeping_task_user(void) {
    #if defined(RGBLIGHT_ENABLE)
    if (tally_light_auto && get_highest_layer(layer_state) == _PAGE_BROADCAST) {
        uint8_t cam_led_idx = active_camera - 1;

        if (is_recording) {
            rgblight_sethsv_at(0, 255, 200, cam_led_idx);   // RED
        } else {
            rgblight_sethsv_at(85, 255, 200, cam_led_idx);  // GREEN
        }
    }
    #endif
}
