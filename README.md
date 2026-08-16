# ReStrike Camera Controller (QMK + Vial Firmware)

Production-ready QMK and Vial firmware for the custom **ReStrike Camera Control Board V1.0**, powered by an **Arduino Pro Micro (ATmega32U4)**.

---

## 🕹️ Hardware Features & Pin Mapping

| Feature / Control | Schematic Label | Pro Micro Pin | ATmega32U4 Port | Description |
| :--- | :--- | :--- | :--- | :--- |
| **Row 0** | `Row0` | Pin 12 | `PB5` | Key Matrix Row 0 |
| **Row 1** | `Row1` | Pin 18 / A1 | `PF6` | Key Matrix Row 1 |
| **Col 0** | `Col0` | Pin 7 | `PD4` | `SW4` (CAM 1), `SW5` (AUX 1) |
| **Col 1** | `Col1` | Pin 8 | `PC6` | `SW6` (CAM 2), `SW7` (AUX 2) |
| **Col 2** | `Col2` | Pin 9 | `PD7` | `SW8` (CAM 3), `SW9` (CAM 4) |
| **Col 3** | `Col3` | Pin 10 | `PE6` | `SW10` (PLAY/PAUSE), `SW11` (START/STOP) |
| **Col 4** | `Col4` | Pin 11 | `PB4` | `Enc1_SW` (Zoom Click), `Enc2_SW` (Shuffle Click) |
| **Encoder 1 (Zoom)** | `Enc1_A`, `Enc1_B` | Pin 17, Pin 16 | `PF7`, `PB1` | Optical / Digital Zoom control |
| **Encoder 2 (Shuffle)**| `Enc2_A`, `Enc2_B` | Pin 15, Pin 14 | `PB3`, `PB2` | Jog / Shuttle Timeline Scrub |
| **Joystick Pan (X)** | `Joystick_H` | Pin 19 / A2 | `PF5` (ADC5) | 10k Pan potentiometer |
| **Joystick Tilt (Y)** | `Joystick_V` | Pin 20 / A3 | `PF4` (ADC4) | 10k Tilt potentiometer |
| **Joystick Button** | `Joystick_Sw` | Pin 2 / RX | `PD2` | Active LOW with pull-up |
| **ARGB LEDs** | `LED_SIG` | Pin 13 | `PB6` | 12x WS2812B / SK6812 chain |
| **OLED Display** | `I2C_SDA`, `I2C_SCL`| Pin 5, Pin 6 | `PD1`, `PD0` | SSD1306 128×64 / 128×128 I2C |
| **Signal / Tally LED** | `SIG_LED` | Pin 1 / TX | `PD3` | Discrete TX tally indicator |

---

## ⚡ 4 Controller Pages (Layers) & Encoder Push Buttons

The controller features **4 switchable operational pages** displayed on the OLED screen. You can cycle between pages at any time by **clicking the ZOOM Encoder (SW1 Push Button)**:

```
[ PAGE 1: OBS BROADCAST ] ──> [ PAGE 2: AUDIO MIXER ] ──> [ PAGE 3: INSTANT REPLAY ] ──> [ PAGE 4: RGB LIGHTING ] ──> (Loop)
```

---

### 📺 Page 1: OBS / Broadcast Studio Mode
* **CAM 1 – CAM 4:** `F13`, `F14`, `F15`, `F16` *(Clean virtual function keys with zero typing conflicts)*
* **AUX 1 – AUX 2:** `F17`, `F18`
* **PLAY / PAUSE:** `Media Play/Pause`
* **START / STOP:** `F19` *(Start/Stop Recording)*
* **ZOOM Knob (SW1):** Rotate $\rightarrow$ `Zoom In` (`Ctrl + =`) / `Zoom Out` (`Ctrl + -`), **Click $\rightarrow$ Cycle to Next Page**
* **SHUFFLE Knob (SW2):** Rotate $\rightarrow$ `Jog / Scrub Timeline` (`Left Arrow` / `Right Arrow`), **Click $\rightarrow$ Audio Mute (`KC_MUTE`)**
* **ARGB Lighting:** Active camera underglow LED glows **🟢 Green (Standby/Preview)** or **🔴 Red (Live Program / Recording)**.
* **OLED Screen:** Shows active camera tally box `[ CAM 1 ]`, live `[REC]` status, and real-time Zoom progress bar.

---

### 🎙️ Page 2: Audio Mixer & Channel Mutes
* **CAM 1 – CAM 4:** **Mute Channel 1**, **Mute Channel 2**, **Mute Channel 3**, **Mute Channel 4** (`Ctrl+Alt+1..4`)
* **AUX 1 – AUX 2:** **Solo Host Mic (`F20`)**, **Solo Guest Mic (`F21`)**
* **PLAY / PAUSE & START / STOP:** **Master Mute**, **Mic Mute**
* **ZOOM Knob:** Rotate $\rightarrow$ **Mic Input Gain (+/-)**, **Click $\rightarrow$ Cycle to Next Page**
* **SHUFFLE Knob:** Rotate $\rightarrow$ **Master Volume (+/-)**, **Click $\rightarrow$ Reset Levels**
* **OLED Screen:** Live channel mute matrix (`C1:ON C2:MUT`) + Master Volume and Mic Gain gauges.

---

### ⏪ Page 3: Instant Replay & Timeline Jog
* **CAM 1 – CAM 4:** **25% Speed**, **50% Speed**, **75% Speed**, **100% Speed**
* **AUX 1 – AUX 2:** **Mark IN (`[`)**, **Mark OUT (`]`)**
* **PLAY / PAUSE:** Play / Pause Replay Clip (`Space`)
* **START / STOP:** **Save Replay Highlight Clip (`F24`)**
* **ZOOM Knob:** Rotate $\rightarrow$ **10-Second Quick Jump**, **Click $\rightarrow$ Cycle to Next Page**
* **SHUFFLE Knob:** Rotate $\rightarrow$ **Single-Frame Precision Scrub**, **Click $\rightarrow$ Jump to Mark IN (`Home`)**
* **OLED Screen:** Playback speed percentage, In/Out marker validation, and shuttle direction indicator.

---

### 🌈 Page 4: ARGB Lighting & Hardware Setup
* **CAM 1:** **Toggle RGB Lighting (ON/OFF)** (`RGB_TOG`)
* **CAM 2 / CAM 3:** **Next Animation Mode** (`RGB_MOD`) / **Previous Animation Mode** (`RGB_RMOD`)
* **CAM 4:** **Toggle Camera Tally Mode (Auto / Manual)**
* **AUX 1 / AUX 2:** **Hue Up (`RGB_HUI`)** / **Hue Down (`RGB_HUD`)**
* **PLAY / PAUSE & START / STOP:** **Saturation Up (`RGB_SAI`)** / **Saturation Down (`RGB_SAD`)**
* **ZOOM Knob:** Rotate $\rightarrow$ **LED Brightness Up / Down**, **Click $\rightarrow$ Cycle to Next Page**
* **SHUFFLE Knob:** Rotate $\rightarrow$ **Live Hue Shift**, **Click $\rightarrow$ Reset Default Rainbow Mode**
* **OLED Screen:** Live RGB status (ON/OFF), Hue value, Saturation value, Brightness gauge, and Tally state.

---

## 🛠️ How to Compile and Flash

### 1. Prerequisites (QMK MSYS on Windows)
1. Download and install **[QMK MSYS](https://msys.qmk.fm/)**.
2. Open QMK MSYS and setup the QMK environment:
   ```bash
   qmk setup
   ```

### 2. Copy Keyboard Files into QMK
Copy the `qmk_firmware/keyboards/restrike_ctr` directory into your local QMK source tree:
```bash
cp -r /path/to/reStrikeCTR/qmk_firmware/keyboards/restrike_ctr ~/qmk_firmware/keyboards/
```

### 3. Compile the Firmware

* **Standard QMK:**
  ```bash
  qmk compile -kb restrike_ctr -km default
  ```

* **Vial Firmware (Graphical UI configuration):**
  ```bash
  qmk compile -kb restrike_ctr -km vial
  ```

### 4. Flash to Arduino Pro Micro
Run the flash command:
```bash
qmk flash -kb restrike_ctr -km vial
```
When prompted `Waiting for bootloader...`, **double-tap the RESET button** (or short `RST` to `GND` twice quickly) on the Arduino Pro Micro. The Caterina bootloader will stay active for 8 seconds and flash automatically.

---

## 🎨 Real-Time Customization with Vial

Once flashed with the `vial` keymap, you can reconfigure the controller at any time with **zero code and no reflashing**:

1. Open **[vial.rocks](https://vial.rocks)** in Google Chrome / Microsoft Edge (or use the offline Vial app).
2. Click **Start** and select `ReStrike Camera Controller`.
3. You can now:
   * Click on any button to assign new keyboard shortcuts, macros, or OBS keys.
   * Customize clockwise/counter-clockwise actions for both rotary encoders per-layer.
   * Change RGB LED colors, brightness, and animations in real time.
   * Changes save instantly to the Pro Micro EEPROM.
