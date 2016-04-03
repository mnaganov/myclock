/* -*- mode: c; -*- */
#include <ADK.h>

#define LED_LEFT_SEPARATOR 9
#define LED_RIGHT_SEPARATOR 2
#define ICON_LOCK 7

#define BTN_MAX 0x1B
#define BTN_LOCK 7

enum State {
  StateClock
};

enum LockState {
  LockedState,
  UnlockedState
};

enum ButtonState {
  ButtonFree,
  ButtonTouched
};

ADK L;

void adkPutchar(char c) {
  Serial.write(c);
}

struct Color {
  uint8_t r, g, b;
};

static void displayCurrentTime(struct Color c) {
  uint16_t year;
  uint8_t month, day, h, m, s;
  L.rtcGet(&year, &month, &day, &h, &m, &s);
  L.ledDrawLetter(0, h / 10 + '0', c.r, c.g, c.b);
  L.ledDrawLetter(1, h % 10 + '0', c.r, c.g, c.b);
  L.ledWrite(LED_LEFT_SEPARATOR, c.r, c.g, c.b);
  L.ledDrawLetter(2, m / 10 + '0', c.r, c.g, c.b);
  L.ledDrawLetter(3, m % 10 + '0', c.r, c.g, c.b);
  L.ledWrite(LED_RIGHT_SEPARATOR, 0, 0, 0);
  L.ledDrawLetter(4, '8', 0, 0, 0);
  L.ledDrawLetter(5, '8', 0, 0, 0);
}

static void displayLockedState(int state, struct Color c) {
  bool locked = state == LockedState;
  L.ledDrawIcon(ICON_LOCK, locked ? c.r : 0, locked ? c.g : 0, locked ? c.b : 0);
}

static void updateButtonStates(int* states) {
  //static uint64_t lastTime[BTN_MAX] = { 0, };
  //static unit32_t lastStates = 0;
  uint32_t currStates = (((uint32_t)L.capSenseButtons()) << 16) | L.capSenseIcons();

  for (uint32_t i = 0, mask = 1; i < BTN_MAX; ++i, mask <<= 1) {
    states[i] = (currStates & mask) == mask ? ButtonTouched : ButtonFree;
  }

  //lastStates = currStates;
}

void setup(void) {
 Serial.begin(115200);
 L.adkSetPutchar(adkPutchar);
 L.adkInit();
}

void loop(void) {
  struct Color color = { 0xff, 0, 0 };
  State state = StateClock;
  LockState locked = UnlockedState;
  int buttonStates[BTN_MAX] = { ButtonFree, };

  while(1) {
    L.adkEventProcess();
    updateButtonStates(buttonStates);

    locked = buttonStates[BTN_LOCK] == ButtonTouched ? LockedState : UnlockedState;
    displayLockedState(locked, color);

    switch (state) {
      case StateClock:
        displayCurrentTime(color);
        break;
    }
  }
}
