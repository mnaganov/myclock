/* -*- mode: c; -*- */
#include <ADK.h>

#define LED_LEFT_SEPARATOR 9
#define LED_RIGHT_SEPARATOR 2
#define ICON_CLOCK 3
#define ICON_LOCK 7

#define BTN_MAX 0x1B
#define BTN_LOCK 7
#define BUTTON_PRESS_TIMEOUT_MS 400
#define BUTTON_HELD_TIMEOUT_MS 2000

enum State {
  StateClock,
  StateSetTime,
};

enum LockState {
  UnlockedState,
  LockedState
};

enum ButtonState {
  ButtonFree,
  ButtonPressed,
  ButtonHeld
};

ADK L;

void adkPutchar(char c) {
  Serial.write(c);
}
extern "C" void dbgPrintf(const char *, ... );

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

static bool isLockedState(int state) {
  return state == StateClock;
}

static void displayLockedState(int state, struct Color c) {
  bool locked = isLockedState(state);
  L.ledDrawIcon(ICON_LOCK, locked ? c.r : 0, locked ? c.g : 0, locked ? c.b : 0);
}

static void updateButtonStates(int* states) {
  static uint64_t lastTime[BTN_MAX] = { 0, };
  static uint32_t lastStates = 0;
  uint32_t currStates = (((uint32_t)L.capSenseButtons()) << 16) | L.capSenseIcons();

  for (uint32_t i = 0, mask = 1; i < BTN_MAX; ++i, mask <<= 1) {
    uint64_t currentTime = L.getUptime();
    if (states[i] == ButtonFree && (lastStates & mask) != mask && (currStates & mask) == mask) {
      // Button touched
      lastTime[i] = currentTime;
    } else if (states[i] == ButtonFree && (currStates & mask) == mask &&
               (currentTime - lastTime[i]) >= BUTTON_PRESS_TIMEOUT_MS) {
      // Button touched long enough to be considered pressed
      states[i] = ButtonPressed;
    } else if (states[i] == ButtonPressed && (currStates & mask) == mask &&
               (currentTime - lastTime[i]) >= BUTTON_HELD_TIMEOUT_MS) {
      // Button touched long enough to be considered held
      states[i] = ButtonHeld;
    } else if (states[i] != ButtonFree && (currStates & mask) != mask) {
      // Button released
      states[i] = ButtonFree;
    }
  }

  lastStates = currStates;
}

void setup(void) {
 Serial.begin(115200);
 L.adkSetPutchar(adkPutchar);
 L.adkInit();
}

void loop(void) {
  struct Color color = { 0xff, 0, 0 };
  State state = StateClock;
  bool waitingForButtonRelease = false;
  int buttonStates[BTN_MAX] = { ButtonFree, };

  while(1) {
    L.adkEventProcess();
    updateButtonStates(buttonStates);

    if (!waitingForButtonRelease) {
      bool locked = isLockedState(state);
      if (locked && buttonStates[BTN_LOCK] == ButtonHeld) {
        state = StateSetTime;
        waitingForButtonRelease = true;
      } else if (!locked && buttonStates[BTN_LOCK] != ButtonFree) {
        state = StateClock;
        waitingForButtonRelease = true;
      }
    } else if (buttonStates[BTN_LOCK] == ButtonFree) {
      waitingForButtonRelease = false;
    }

    displayLockedState(state, color);
    switch (state) {
      case StateClock:
      case StateSetTime:
        displayCurrentTime(color);
        break;
    }
  }
}
