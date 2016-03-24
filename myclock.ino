// -*- mode: c-mode; -*-
#include <ADK.h>

ADK L;

void adkPutchar(char c) {
  Serial.write(c);
}

static void clearScreen(void) {
  uint32_t i;
  for(i = 0; i < NUM_LEDS; i++) {
    L.ledWrite(i, 0, 0, 0);
  }
}

void setup(void) {
 Serial.begin(115200);
 L.adkSetPutchar(adkPutchar);
 L.adkInit();
}

void loop(void) {
  uint8_t r, g, b;
  r = 0xff; g = 0; b = 0;
  while(1) {
    L.adkEventProcess();
    uint16_t year;
    uint8_t month, day, h, m, s;
    L.rtcGet(&year, &month, &day, &h, &m, &s);
    L.ledDrawLetter(0, h / 10 + '0', r, g, b);
    L.ledDrawLetter(1, h % 10 + '0', r, g, b);
    L.ledDrawLetter(2, m / 10 + '0', r, g, b);
    L.ledDrawLetter(3, m % 10 + '0', r, g, b);
    L.ledDrawLetter(4, s / 10 + '0', r, g, b);
    L.ledDrawLetter(5, s % 10 + '0', r, g, b);
  }
}
