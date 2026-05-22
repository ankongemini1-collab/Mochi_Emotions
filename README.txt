# Mochi Emotions + QGIF Animations

For NodeMCU / ESP8266 + 128x64 SH1106 OLED.

## Included emotions
- angry: from angry.zip
- angry2: from angry2.zip
- angry3: from angry3.zip
- sneeze: converted from action_sneeze_01.qgif
- devil: converted from emotion_devil_02.qgif

## How to use
1. Extract this ZIP.
2. Open `Mochi_Emotions.ino` in Arduino IDE.
3. Keep all `.h` files in the same folder as the `.ino`.
4. Select your NodeMCU / ESP8266 board.
5. Upload.

## QGIF note
The two QGIF files had this structure:
- 1 byte: frame count
- 2 bytes: width
- 2 bytes: height
- frame-count × 2 bytes: frame delays
- then 1024 bytes per frame

Their pixel data was row-major horizontal 1-bit, MSB-first.
I converted them into SSD1306/SH1106 page-based 1024-byte frames and inverted the colors so the face becomes white on black.

## Make the face smaller
In `Mochi_Emotions.ino`, change:

```cpp
const int DRAW_W = 128;
const int DRAW_H = 64;
```

Good smaller values:

```cpp
const int DRAW_W = 116;
const int DRAW_H = 58;
```

or:

```cpp
const int DRAW_W = 110;
const int DRAW_H = 55;
```

Keep the ratio close to 2:1, otherwise the face gets stretched.

## Test only one emotion
Inside `loop()`, comment out the animations you do not want.
For example, to test only devil:

```cpp
void loop() {
  playAnimation(devilFrames, DEVIL_FRAME_COUNT, DEVIL_DELAY_MS);
  delay(1200);
}
```

## If upload/compile becomes too heavy
This sketch contains 5 animations, so it uses more flash memory. NodeMCU should usually handle it, but if Arduino IDE complains, remove one or two `#include` lines and their `playAnimation(...)` calls.
