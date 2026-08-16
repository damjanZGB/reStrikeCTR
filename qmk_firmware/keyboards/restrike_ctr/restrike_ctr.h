#pragma once

#include "quantum.h"

/* Layout Macro
 *
 * Physical arrangement:
 * Row 0: [CAM 1]    [CAM 2]    [CAM 3]    [CAM 4]
 * Row 1: [AUX 1]    [AUX 2]
 * Bottom:           [PLAY/PAUSE]   [START/STOP]
 * Knobs: [ZOOM CLICK]                         [SHUFFLE CLICK]
 */
#define LAYOUT( \
    k00, k01, k02, k12, \
    k10, k11,           \
    k03, k13,           \
    k04, k14            \
) { \
    { k00, k01, k02, k03, k04 }, \
    { k10, k11, k12, k13, k14 }  \
}

/* Shared state exported by restrike_ctr.c for use in keymaps */
extern int16_t  joy_x_val;
extern int16_t  joy_y_val;
extern bool     joy_btn_state;
extern bool     obs_connected;

/* Upstream HID senders (defined in restrike_ctr.c, guarded by RAW_ENABLE) */
#ifdef RAW_ENABLE
void restrike_send_handshake(void);
void restrike_send_heartbeat(void);
void restrike_send_key_event(uint8_t key_idx, bool pressed);
void restrike_send_encoder_event(uint8_t enc_idx, int8_t direction, uint8_t detents);
void restrike_send_joystick_state(int16_t x, int16_t y, bool btn);
#endif
