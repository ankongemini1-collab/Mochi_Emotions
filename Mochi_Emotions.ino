#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <pgmspace.h>

#include "emotion_angry3.h"
#include "emotion_sneeze.h"
#include "emotion_devil.h"

// ================= OLED SETUP =================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_I2C_ADDRESS 0x3C
#define OLED_RESET_PIN -1
#define BUZZER_PIN D6

Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET_PIN);

// ================= FACE SIZE =================
const int DRAW_W = 128;
const int DRAW_H = 64;

// If you want slightly smaller later, try:
// const int DRAW_W = 116;
// const int DRAW_H = 58;

const uint16_t DEFAULT_FRAME_DELAY_MS = 70;
const unsigned long SNEEZE_SOUND_LENGTH_MS = 5500;

unsigned long sneezeSoundStart = 0;
int currentBuzzerFreq = -1;

void setBuzzer(int freq) {
  if (freq <= 0) {
    if (currentBuzzerFreq != 0) {
      noTone(BUZZER_PIN);
      currentBuzzerFreq = 0;
    }
  } else if (currentBuzzerFreq != freq) {
    tone(BUZZER_PIN, freq);
    currentBuzzerFreq = freq;
  }
}

int slideFreq(unsigned long t, unsigned long startT, unsigned long endT, int startFreq, int endFreq) {
  if (t <= startT) return startFreq;
  if (t >= endT) return endFreq;

  float progress = (float)(t - startT) / (float)(endT - startT);
  return startFreq + (int)((endFreq - startFreq) * progress);
}

int warbleFreq(unsigned long t, int baseFreq, int amount, int speedMs) {
  if (((t / speedMs) % 2) == 0) {
    return baseFreq + amount;
  }

  return baseFreq - amount;
}

// Reads one pixel from a 128x64 page-based OLED frame
bool getPageBasedPixel(const uint8_t *frame, int x, int y) {
  if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return false;

  int byteIndex = (y / 8) * SCREEN_WIDTH + x;
  uint8_t b = pgm_read_byte(frame + byteIndex);

  return b & (1 << (y & 7));
}

void drawFrame(const uint8_t *frame) {
  display.clearDisplay();

  int xOffset = (SCREEN_WIDTH - DRAW_W) / 2;
  int yOffset = (SCREEN_HEIGHT - DRAW_H) / 2;

  for (int y = 0; y < DRAW_H; y++) {
    int srcY = y * SCREEN_HEIGHT / DRAW_H;

    for (int x = 0; x < DRAW_W; x++) {
      int srcX = x * SCREEN_WIDTH / DRAW_W;

      if (getPageBasedPixel(frame, srcX, srcY)) {
        display.drawPixel(x + xOffset, y + yOffset, SH110X_WHITE);
      }
    }

    yield();
  }

  display.display();
}

void playAnimation(const uint8_t frames[][1024], uint16_t frameCount, uint16_t frameDelay) {
  for (uint16_t i = 0; i < frameCount; i++) {
    drawFrame(frames[i]);
    delay(frameDelay);
  }
}

uint16_t getAngry3ToneForFrame(uint16_t frameIndex) {
  // 57 frames * 70 ms = 3990 ms total.
  // Visual timing:
  // 0-5    neutral
  // 6-11   flare-up
  // 12-49  sustained angry hold
  // 50-56  snap back / cool down
  switch (frameIndex) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return 0;

    case 6:
      return 110;   // A2
    case 7:
      return 123;   // B2
    case 8:
      return 147;   // D3
    case 9:
      return 175;   // F3
    case 10:
      return 196;   // G3
    case 11:
      return 247;   // B3

    case 12:
    case 14:
    case 16:
    case 18:
    case 20:
    case 22:
      return 123;   // first simmer
    case 13:
    case 15:
    case 17:
    case 19:
    case 21:
    case 23:
      return 110;   // low growl

    case 24:
    case 26:
    case 28:
    case 30:
    case 32:
    case 34:
      return 147;   // anger rising
    case 25:
    case 27:
    case 29:
    case 31:
    case 33:
    case 35:
      return 131;   // tighter grind

    case 36:
    case 38:
    case 41:
    case 43:
    case 46:
      return 175;   // hotter rage
    case 37:
    case 39:
    case 42:
    case 45:
    case 47:
      return 165;   // hard pulse

    case 40:
    case 44:
    case 48:
    case 49:
      return 294;   // sharp spike, almost snapping

    case 50:
      return 185;   // F#3
    case 51:
      return 147;   // D3
    case 52:
      return 123;   // B2
    case 53:
      return 110;   // A2
    case 54:
      return 0;
    case 55:
      return 98;    // final grumble
    case 56:
      return 0;
  }

  return 0;
}

uint16_t getAngry3ToneDurationForFrame(uint16_t frameIndex) {
  if (frameIndex <= 5) {
    return 0;
  }

  if (frameIndex <= 11) {
    return 60;
  }

  if (frameIndex <= 23) {
    return 40;
  }

  if (frameIndex <= 35) {
    return 46;
  }

  if (frameIndex <= 49) {
    if (frameIndex == 40 || frameIndex == 44 || frameIndex == 48 || frameIndex == 49) {
      return 66;
    }

    return 52;
  }

  if (frameIndex == 55) {
    return 44;
  }

  return 40;
}

void playAngry3AnimationWithMusic() {
  for (uint16_t i = 0; i < ANGRY3_FRAME_COUNT; i++) {
    uint16_t toneHz = getAngry3ToneForFrame(i);
    uint16_t toneMs = getAngry3ToneDurationForFrame(i);

    if (toneHz > 0) {
      tone(BUZZER_PIN, toneHz, toneMs);
    } else {
      noTone(BUZZER_PIN);
    }

    drawFrame(angry3Frames[i]);
    delay(DEFAULT_FRAME_DELAY_MS);
  }

  noTone(BUZZER_PIN);
}

void startSneezeSound() {
  sneezeSoundStart = millis();
  currentBuzzerFreq = -1;
}

void updateSneezeSound() {
  unsigned long t = millis() - sneezeSoundStart;
  int freq = 0;

  if (t < 80) freq = 520;
  else if (t < 320) freq = 0;
  else if (t < 400) freq = 640;
  else if (t < 740) freq = 0;
  else if (t < 840) freq = 760;
  else if (t < 1100) freq = 0;
  else if (t < 1820) freq = slideFreq(t, 1100, 1820, 560, 1180);
  else if (t < 1900) freq = 0;
  else if (t < 2040) freq = warbleFreq(t, 1320, 90, 25);
  else if (t < 2090) freq = 0;
  else if (t < 2390) freq = slideFreq(t, 2090, 2390, 980, 420);
  else if (t < 2440) freq = 0;
  else if (t < 2500) freq = 330;
  else if (t < 2620) freq = 0;
  else if (t < 2680) freq = 430;
  else if (t < 2800) freq = 0;
  else if (t < 2870) freq = 310;
  else if (t < 3000) freq = 0;
  else if (t < 3060) freq = 460;
  else if (t < 3180) freq = 0;
  else if (t < 3250) freq = 350;
  else if (t < 3340) freq = 0;
  else if (t < 3380) freq = 320;
  else if (t < 3420) freq = 0;
  else if (t < 3460) freq = 390;
  else if (t < 3500) freq = 0;
  else if (t < 3540) freq = 470;
  else if (t < 3580) freq = 0;
  else if (t < 3620) freq = 410;
  else if (t < 3660) freq = 0;
  else if (t < 3700) freq = 300;
  else if (t < 3740) freq = 0;
  else if (t < 3780) freq = 420;
  else if (t < 3820) freq = 0;
  else if (t < 3860) freq = 520;
  else if (t < 3900) freq = 0;
  else if (t < 3940) freq = 450;
  else if (t < 4010) freq = 0;
  else if (t < 4350) freq = slideFreq(t, 4010, 4350, 280, 880);
  else if (t < 4400) freq = 0;
  else if (t < 4720) freq = slideFreq(t, 4400, 4720, 900, 520);
  else if (t < 4780) freq = 0;
  else if (t < 4930) freq = warbleFreq(t, 650, 80, 35);
  else if (t < 5000) freq = 0;
  else if (t < 5060) freq = 560;
  else if (t < 5120) freq = 0;
  else if (t < 5180) freq = 760;
  else if (t < 5240) freq = 0;
  else if (t < 5310) freq = 620;
  else if (t < 5370) freq = 0;
  else if (t < 5500) freq = slideFreq(t, 5370, 5500, 780, 480);
  else freq = 0;

  setBuzzer(freq);
}

void smartSoundDelay(unsigned long delayTime) {
  unsigned long startTime = millis();

  while (millis() - startTime < delayTime) {
    updateSneezeSound();
    delay(2);
    yield();
  }
}

void playSneezeAnimationWithMusic() {
  startSneezeSound();

  for (uint16_t i = 0; i < SNEEZE_FRAME_COUNT; i++) {
    drawFrame(sneezeFrames[i]);
    smartSoundDelay(SNEEZE_DELAY_MS);
  }

  while (millis() - sneezeSoundStart < SNEEZE_SOUND_LENGTH_MS) {
    updateSneezeSound();
    delay(2);
    yield();
  }

  noTone(BUZZER_PIN);
  currentBuzzerFreq = 0;
}

uint16_t getDevilToneForFrame(uint16_t frameIndex) {
  // 61 frames * 60 ms = 3660 ms total.
  // Arc:
  // 0-5   neutral intro
  // 6-17  transformation climb
  // 18-43 evil grin hold
  // 44-60 return / resolve
  switch (frameIndex) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
      return 0;

    case 6:
    case 7:
      return 392;   // G4
    case 8:
    case 9:
      return 466;   // A#4
    case 10:
    case 11:
      return 587;   // D5
    case 12:
    case 13:
      return 698;   // F5
    case 14:
    case 15:
      return 784;   // G5
    case 16:
    case 17:
      return 880;   // A5

    case 18:
    case 21:
    case 24:
    case 27:
    case 30:
    case 33:
    case 36:
    case 39:
    case 42:
      return 740;   // F#5

    case 19:
    case 22:
    case 25:
    case 28:
    case 31:
    case 34:
    case 37:
    case 40:
    case 43:
      return 698;   // F5

    case 20:
    case 23:
    case 26:
    case 29:
    case 32:
    case 35:
    case 38:
    case 41:
      return 622;   // D#5

    case 44:
    case 45:
      return 659;   // E5
    case 46:
    case 47:
      return 587;   // D5
    case 48:
    case 49:
      return 523;   // C5
    case 50:
    case 51:
      return 466;   // A#4
    case 52:
    case 53:
      return 392;   // G4
    case 54:
    case 55:
      return 330;   // E4
    case 56:
      return 294;   // D4
    case 57:
      return 262;   // C4
    case 58:
    case 59:
      return 0;
    case 60:
      return 330;   // small cheeky end ping
  }

  return 0;
}

void playDevilAnimationWithMusic() {
  for (uint16_t i = 0; i < DEVIL_FRAME_COUNT; i++) {
    uint16_t toneHz = getDevilToneForFrame(i);

    if (toneHz > 0) {
      tone(BUZZER_PIN, toneHz, DEVIL_DELAY_MS - 5);
    } else {
      noTone(BUZZER_PIN);
    }

    drawFrame(devilFrames[i]);
    delay(DEVIL_DELAY_MS);
  }

  noTone(BUZZER_PIN);
}

void setup() {
  Wire.begin(D2, D1);   // SDA = D2, SCL = D1
  Wire.setClock(400000);
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  if (!display.begin(OLED_I2C_ADDRESS, true)) {
    while (true) {
      delay(10);
    }
  }

  display.clearDisplay();
  display.setRotation(0);
  display.display();
}

void loop() {
  playAngry3AnimationWithMusic();
  delay(900);

  playSneezeAnimationWithMusic();
  delay(900);

  playDevilAnimationWithMusic();
  delay(1200);
}
