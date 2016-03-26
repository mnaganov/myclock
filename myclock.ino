/* -*- mode: c; -*- */
#include <ADK.h>

#define LED_LEFT_SEPARATOR 9
#define LED_RIGHT_SEPARATOR 2
#define ICON_LOCK 7

ADK L;

void adkPutchar(char c) {
  Serial.write(c);
}

static void displayCurrentTime(uint8_t r, uint8_t g, uint8_t b) {
  uint16_t year;
  uint8_t month, day, h, m, s;
  L.rtcGet(&year, &month, &day, &h, &m, &s);
  L.ledDrawLetter(0, h / 10 + '0', r, g, b);
  L.ledDrawLetter(1, h % 10 + '0', r, g, b);
  L.ledWrite(LED_LEFT_SEPARATOR, r, g, b);
  L.ledDrawLetter(2, m / 10 + '0', r, g, b);
  L.ledDrawLetter(3, m % 10 + '0', r, g, b);
  L.ledWrite(LED_RIGHT_SEPARATOR, 0, 0, 0);
  L.ledDrawLetter(4, '8', 0, 0, 0);
  L.ledDrawLetter(5, '8', 0, 0, 0);
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
    displayCurrentTime(r, g, b);
    L.ledDrawIcon(ICON_LOCK, r, g, b);
  }
}
