#ifndef APPLICATION_H
#define APPLICATION_H

#include "clock.h"
#include "computer.h"
#include "arm_button.h"

extern Clock        AO_Clock;
extern Computer     AO_Computer;
extern ArmButton    AO_ArmButton;

enum CoffeeClockSignals {
    ROTARY_INC_SIG = Q_USER_SIG,
    ROTARY_DEC_SIG,
    ROTARY_PUSH_SIG,
    DISPLAY_SET_SIG,
    SET_TIME_SIG,
    TIME_UPDATED_SIG,
    COMPUTER_ALIVE_SIG,
    COMPUTER_DISCONNECTED_SIG,
    TURN_ON_SIG,
    TURN_OFF_SIG,
    ARM_BUTTON_PRESS_SIG,
    ARM_BUTTON_RELEASE_SIG,
    ALARM_SET_SIG,
    ALARM_SET_FROM_COMPUTER_SIG,
    MAX_SIG
};

//============================================================================
// various constants for the application...
enum {
    BSP_TICKS_PER_SEC = 100, // number of system clock ticks in one second
    SWITCH_ACTIVATE = 5,     // D5 = PD5
    SWITCH_ONOFF = 12,       // D12 = PB4
    LED_ACTIVATED = 6,
    LED_CONNECTED = 7,
    LED_TIME = 8,
    LED_ALARM = 9,
    ALARM_SET_BLINK_TIME = (BSP_TICKS_PER_SEC/2),
    BAUD_RATE = 9600,
    COMPUTER_SIGNAL_TIMEOUT = (unsigned int)(BSP_TICKS_PER_SEC*1.5),
    ARM_BUTTON_TIMEOUT = (unsigned int)(BSP_TICKS_PER_SEC*0.1),
    SERIAL_BUFFER_SIZE = 30,
};

void BSP_displayTime(uint8_t hours, uint8_t minutes);

void BSP_displayHideHours();

void BSP_displayHideMinutes();

void BSP_displayDashes();

void BSP_displayOff();

#endif // APPLICATION_H
