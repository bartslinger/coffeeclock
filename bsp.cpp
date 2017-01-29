#include "qpn.h"
#include "bsp.h"
#include "Arduino.h"
#include <TM1637Display.h>

static TM1637Display display(11, 10);

void BSP_displayTime(uint8_t hours, uint8_t minutes) {
    display.showNumberDecEx(hours*100+minutes, 0x40, true);
}

void BSP_displayHideHours() {
    uint8_t data[] = {0x00, 0x80};
    display.setSegments(data, 2, 0);
}

void BSP_displayHideMinutes() {
    uint8_t data[] = {0x00, 0x00};
    display.setSegments(data, 2, 2);
}

void BSP_displayDashes() {
    uint8_t data[] = {SEG_G, SEG_G, SEG_G, SEG_G};
    display.setSegments(data);
}

void BSP_displayOff() {
    uint8_t data[] = {0x00, 0x00, 0x00, 0x00};
    display.setSegments(data);
}

void BSP_displayEnable() {
    display.setBrightness(0x0f);
}

void BSP_checkLdrValue(uint16_t analogIn) {
    display.showNumberDec(analogIn);
}
