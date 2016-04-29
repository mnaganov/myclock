/* -*- mode: c; -*- */
#include <ADK.h>

#define LED_HOURS_10 0
#define LED_HOURS_1 1
#define LED_MINUTES_10 2
#define LED_MINUTES_1 3
#define LED_SECONDS_10 4
#define LED_SECONDS_1 5
#define LED_LEFT_SEPARATOR 9
#define LED_RIGHT_SEPARATOR 2
#define ICON_CLOCK 0
#define ICON_LOCK 7
#define BLINK_VISIBLE_TIME_MS 1000
#define BLINK_HIDDEN_TIME_MS 200

#define BTN_HOURS_10_UP 0x1A
#define BTN_HOURS_10_DN 0x1B
#define BTN_HOURS_1_UP 0x18
#define BTN_HOURS_1_DN 0x19
#define BTN_MINUTES_10_UP 0x16
#define BTN_MINUTES_10_DN 0x17
#define BTN_MINUTES_1_UP 0x14
#define BTN_MINUTES_1_DN 0x15
#define BTN_MAX 0x1B
#define BTN_CLOCK 3
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
  ButtonHeld,
  // Waiting for button release
  ButtonProcessed
};

ADK L;

void adkPutchar(char c) {
  Serial.write(c);
}
extern "C" void dbgPrintf(const char *, ... );

struct Color {
  uint8_t r, g, b;
};

static void displayCurrentTime(int state, struct Color c, uint8_t h, uint8_t m) {
  static uint64_t lastTime = 0;
  static bool visible = true;

  if (state == StateSetTime) {
    uint64_t currentTime = L.getUptime();
    if (visible && (currentTime - lastTime) >= BLINK_VISIBLE_TIME_MS) {
      visible = false;
      lastTime = currentTime;
    } else if (!visible && (currentTime - lastTime) >= BLINK_HIDDEN_TIME_MS) {
      visible = true;
      lastTime = currentTime;
    }
  } else {
    visible = true;
  }

  if (visible) {
    L.ledDrawLetter(LED_HOURS_10, h / 10 + '0', c.r, c.g, c.b);
    L.ledDrawLetter(LED_HOURS_1, h % 10 + '0', c.r, c.g, c.b);
    L.ledWrite(LED_LEFT_SEPARATOR, c.r, c.g, c.b);
    L.ledDrawLetter(LED_MINUTES_10, m / 10 + '0', c.r, c.g, c.b);
    L.ledDrawLetter(LED_MINUTES_1, m % 10 + '0', c.r, c.g, c.b);
  } else {
    L.ledDrawLetter(LED_HOURS_10, '8', 0, 0, 0);
    L.ledDrawLetter(LED_HOURS_1, '8', 0, 0, 0);
    L.ledWrite(LED_LEFT_SEPARATOR, 0, 0, 0);
    L.ledDrawLetter(LED_MINUTES_10, '8', 0, 0, 0);
    L.ledDrawLetter(LED_MINUTES_1, '8', 0, 0, 0);
  }
  L.ledWrite(LED_RIGHT_SEPARATOR, 0, 0, 0);
  L.ledDrawLetter(LED_SECONDS_10, '8', 0, 0, 0);
  L.ledDrawLetter(LED_SECONDS_1, '8', 0, 0, 0);
}

static bool isLockedState(int state) {
  return state == StateClock;
}

static void displayIcons(int state, struct Color c) {
  bool settingTime = state == StateSetTime;
  L.ledDrawIcon(ICON_CLOCK, settingTime ? c.r : 0, settingTime ? c.g : 0, settingTime ? c.b : 0);
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
  int buttonStates[BTN_MAX] = { ButtonFree, };
  uint8_t setH, setM;

  while(1) {
    L.adkEventProcess();
    updateButtonStates(buttonStates);

    bool locked = isLockedState(state);
    if (locked && buttonStates[BTN_LOCK] == ButtonHeld) {
      state = StateSetTime;
      uint16_t year;
      uint8_t month, day, s;
      L.rtcGet(&year, &month, &day, &setH, &setM, &s);
      buttonStates[BTN_LOCK] = ButtonProcessed;
    } else if (!locked && buttonStates[BTN_LOCK] == ButtonPressed) {
      state = StateClock;
      buttonStates[BTN_LOCK] = ButtonProcessed;
    }

    if (state == StateSetTime) {
      if (buttonStates[BTN_MINUTES_1_UP] == ButtonPressed) {
        if (setM++ == 59) {
          setM = 0;
          if (setH++ == 23) {
            setH = 0;
          }
        }
        buttonStates[BTN_MINUTES_1_UP] = ButtonProcessed;
      }
      if (buttonStates[BTN_MINUTES_1_DN] == ButtonPressed) {
        if (setM-- == 0) {
          setM = 59;
          if (setH-- == 0) {
            setH = 23;
          }
        }
        buttonStates[BTN_MINUTES_1_DN] = ButtonProcessed;
      }
      if (buttonStates[BTN_HOURS_1_UP] == ButtonPressed) {
        if (setH++ == 23) {
          setH = 0;
        }
        buttonStates[BTN_HOURS_1_UP] = ButtonProcessed;
      }
      if (buttonStates[BTN_HOURS_1_DN] == ButtonPressed) {
        if (setH-- == 0) {
          setH = 23;
        }
        buttonStates[BTN_HOURS_1_DN] = ButtonProcessed;
      }
      if (buttonStates[BTN_CLOCK] == ButtonPressed) {
        uint16_t year;
        uint8_t month, day, h, m, s;
        L.rtcGet(&year, &month, &day, &h, &m, &s);
        L.rtcSet(year, month, day, setH, setM, 0);
        state = StateClock;
        buttonStates[BTN_CLOCK] = ButtonProcessed;
      }
    }

    switch (state) {
      case StateClock: {
        uint16_t year;
        uint8_t month, day, h, m, s;
        L.rtcGet(&year, &month, &day, &h, &m, &s);
        displayCurrentTime(state, color, h, m);
        break;
      }
      case StateSetTime: {
        displayCurrentTime(state, color, setH, setM);
        break;
      }
    }
    displayIcons(state, color);
  }
}
