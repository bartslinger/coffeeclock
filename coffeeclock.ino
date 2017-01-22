/*****************************************************************************
* Model: coffeeclock.qm
* File:  ./coffeeclock.ino
*
* This code has been generated by QM tool (see state-machine.com/qm).
* DO NOT EDIT THIS FILE MANUALLY. All your changes will be lost.
*
* This program is open source software: you can redistribute it and/or
* modify it under the terms of the GNU General Public License as published
* by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful, but
* WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
* or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
* for more details.
*****************************************************************************/
/*${.::coffeeclock.ino} ....................................................*/
#include "qpn.h"
#include "Arduino.h"
#include <EEPROM.h>
#include <TM1637Display.h>

Q_DEFINE_THIS_MODULE("coffeeclock")

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
// Declare Active Objects

#if ((QP_VERSION < 580) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8)))
#error qpn version 5.8.0 or higher required
#endif

/*${AOs::Clock} ............................................................*/
typedef struct Clock {
/* protected: */
    QActive super;

/* private: */
    uint8_t alarm_hours;
    uint8_t alarm_minutes;
    uint8_t time_hours;
    uint8_t time_minutes;
} Clock;

/* protected: */
static QState Clock_initial(Clock * const me);
static QState Clock_on(Clock * const me);
static QState Clock_alarm(Clock * const me);
static QState Clock_set_minutes(Clock * const me);
static QState Clock_minutes_blink_on(Clock * const me);
static QState Clock_minutes_blink_off(Clock * const me);
static QState Clock_show_alarm(Clock * const me);
static QState Clock_set_hours(Clock * const me);
static QState Clock_hours_blink_on(Clock * const me);
static QState Clock_hours_blink_off(Clock * const me);
static QState Clock_time(Clock * const me);
static QState Clock_disconnected(Clock * const me);
static QState Clock_off(Clock * const me);

/*${AOs::Computer} .........................................................*/
typedef struct Computer {
/* protected: */
    QActive super;
} Computer;

/* protected: */
static QState Computer_initial(Computer * const me);
static QState Computer_disconnected(Computer * const me);
static QState Computer_connected(Computer * const me);

/*${AOs::ArmButton} ........................................................*/
typedef struct ArmButton {
/* protected: */
    QActive super;
} ArmButton;

/* protected: */
static QState ArmButton_initial(ArmButton * const me);
static QState ArmButton_unpressed(ArmButton * const me);
static QState ArmButton_pressed(ArmButton * const me);
static QState ArmButton_waiting_for_release(ArmButton * const me);
static QState ArmButton_after_release(ArmButton * const me);


// Instantiate Active Objects and event queues
Clock         AO_Clock;
static QEvt   l_clockQueue[10];

Computer      AO_Computer;
static QEvt   l_computerQueue[10];

ArmButton     AO_ArmButton;
static QEvt   l_armButtonQueue[2];

//============================================================================
// QF_active[] array defines all active object control blocks
QActiveCB const Q_ROM QF_active[] = {
    { (QActive *)0,                (QEvt *)0,              0U                             },
    { (QActive *)&AO_Clock,        l_clockQueue,           Q_DIM(l_clockQueue)            },
    { (QActive *)&AO_Computer,     l_computerQueue,        Q_DIM(l_computerQueue)         },
    { (QActive *)&AO_ArmButton,    l_armButtonQueue,       Q_DIM(l_armButtonQueue)        },
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

//============================================================================

TM1637Display display(11, 10);

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

struct serialBuffer {
    char data[SERIAL_BUFFER_SIZE];
    uint8_t idx = 0;
} sBuf;

void parseByte(uint8_t byte) {
    sBuf.data[sBuf.idx] = byte;
    /* Ignore carriage return */
    if (byte == '\r') {
        return;
    }
    sBuf.idx++;
    if (byte == '\n') {
        /* Parse message */
        if (strncmp(sBuf.data, "TIME=", 5) == 0) {
            if (sBuf.idx == 11 && sBuf.data[7] == ':') {
                char number[] = "00";
                number[0] = sBuf.data[5];
                number[1] = sBuf.data[6];
                uint8_t hours = atoi(number);
                number[0] = sBuf.data[8];
                number[1] = sBuf.data[9];
                uint8_t minutes = atoi(number);
                uint32_t param = (hours << 8) | minutes;
                QACTIVE_POST_X_ISR((QActive *)&AO_Clock, 1U, SET_TIME_SIG, param);
                QACTIVE_POST_X_ISR((QActive *)&AO_Computer, 1U, COMPUTER_ALIVE_SIG, 0U);
            }
        }
        else if (strncmp(sBuf.data, "ARMED=", 6) == 0) {
            bool armed = (sBuf.data[6] == '1');
            digitalWrite(LED_ACTIVATED, armed);
        }
        else if (strncmp(sBuf.data, "CONNECTED=", 10) == 0) {
            bool connected = (sBuf.data[10] == '1');
            digitalWrite(LED_CONNECTED, connected);
        }
        else if (strncmp(sBuf.data, "ALARM=", 6) == 0) {
            if (sBuf.idx == 12 && sBuf.data[8] == ':') {
                char number[] = "00";
                number[0] = sBuf.data[6];
                number[1] = sBuf.data[7];
                uint8_t hours = atoi(number);
                number[0] = sBuf.data[9];
                number[1] = sBuf.data[10];
                uint8_t minutes = atoi(number);
                uint32_t param = (hours << 8) | minutes;
                QACTIVE_POST_X_ISR((QActive *)&AO_Clock, 1U, ALARM_SET_FROM_COMPUTER_SIG, param);
            }
        }
        sBuf.idx = 0;
    }
    if (sBuf.idx == SERIAL_BUFFER_SIZE) {
        sBuf.idx = 0;
    }
}

//============================================================================
// Setup
void setup() {
    // initialize the QF-nano framework
    QF_init(Q_DIM(QF_active));

    // Initialize all active objects
    QActive_ctor(&AO_Clock.super, Q_STATE_CAST(&Clock_initial));
    QActive_ctor(&AO_Computer.super, Q_STATE_CAST(&Computer_initial));
    QActive_ctor(&AO_ArmButton.super, Q_STATE_CAST(&ArmButton_initial));

    /* Initialize LEDs */
    pinMode(LED_ACTIVATED, OUTPUT);
    digitalWrite(LED_ACTIVATED, LOW);
    pinMode(LED_CONNECTED, OUTPUT);
    digitalWrite(LED_CONNECTED, LOW);
    pinMode(LED_TIME, OUTPUT);
    digitalWrite(LED_TIME, LOW);
    pinMode(LED_ALARM, OUTPUT);
    digitalWrite(LED_ALARM, LOW);

    /* Initialize switches */
    pinMode(SWITCH_ACTIVATE, INPUT);
    pinMode(SWITCH_ONOFF, INPUT);

    display.setBrightness(0x0f);

    Serial.begin(BAUD_RATE);   // set the highest stanard baud rate of 115200 bps
}

//============================================================================
// Main loop
void loop() {
    QF_run();
}
//============================================================================
// interrupts...
ISR(TIMER2_COMPA_vect) {
    QF_tickXISR(0); // process time events for tick rate 0
    while (Serial.available() > 0) {
        parseByte(Serial.read());
    }
//    digitalWrite(LED_ACTIVATED, !digitalRead(SWITCH_ACTIVATE));
//    digitalWrite(LED_CONNECTED, digitalRead(SWITCH_ONOFF));
}

volatile bool aFlag = 0U;
volatile bool bFlag = 0U;

ISR(INT0_vect) {
    //rotaryInterrupt();
    cli();
    uint8_t reading = PIND & 0xC;
    if (reading == 0xC && aFlag) {
        QACTIVE_POST_X_ISR((QActive *)&AO_Clock, 1U, ROTARY_DEC_SIG, 0U);
        aFlag = false;
        bFlag = false;
    }
    else if (reading == (1 << 2)) {
        bFlag = true;
    }
    sei();
}

ISR(INT1_vect) {
    //rotaryInterrupt();
    cli();
    uint8_t reading = PIND & 0xC;
    if (reading == 0xC && bFlag) {
        QACTIVE_POST_X_ISR((QActive *)&AO_Clock, 1U, ROTARY_INC_SIG, 0U);
        aFlag = false;
        bFlag = false;
    }
    else if (reading == (1 << 3)) {
        aFlag = true;
    }
    sei();
}

ISR(PCINT1_vect){
    if ((PINC & _BV(DDC1)) == 0) {
        QACTIVE_POST_X_ISR((QActive *)&AO_Clock, 1U, ROTARY_PUSH_SIG, 0U);
    }
}

ISR(PCINT2_vect){
    if ((PIND & _BV(DDD5)) == 0) {
        QACTIVE_POST_X_ISR((QActive *)&AO_ArmButton, 1U, ARM_BUTTON_PRESS_SIG, 0U);
    }
    else {
        //digitalWrite(LED_ACTIVATED, LOW);
        QACTIVE_POST_X_ISR((QActive *)&AO_ArmButton, 1U, ARM_BUTTON_RELEASE_SIG, 0U);
    }
}

//============================================================================
// QF callbacks...
void QF_onStartup(void) {
    // set Timer2 in CTC mode, 1/1024 prescaler, start the timer ticking...
    TCCR2A = (1U << WGM21) | (0U << WGM20);
    TCCR2B = (1U << CS22 ) | (1U << CS21) | (1U << CS20); // 1/2^10
    ASSR  &= ~(1U << AS2);
    TIMSK2 = (1U << OCIE2A); // enable TIMER2 compare Interrupt
    TCNT2  = 0U;

    // set the output-compare register based on the desired tick frequency
    OCR2A  = (F_CPU / BSP_TICKS_PER_SEC / 1024U) - 1U;

    // Configure the rotary knob push button, it has a pull-up on the PCB
    DDRC &= ~(1 << DDC1);
    PORTC &= ~(1 << DDC1);

    // Enable pin change interrupt on the push-button of the rotary knob
    PCICR |= _BV(PCIE1);
    PCMSK1 |= _BV(PCINT9);

    // Enable pin change interrupt on the arm button
    PCICR |= _BV(PCIE2);
    PCMSK2 |=_BV(PCINT21);

    // Configure rotary knob pins as inputs, no pull-ups
    DDRD &= ~((1 << DDD2) | (1 << DDD3));
    PORTD &= ~((1 << DDD2) | (1 << DDD3));

    // Enable external interrupts
    EICRA |= ((1 << ISC00) | (1 << ISC01) | (1 << ISC10) | (1 << ISC11));
    EIMSK |= ((1 << INT0) | (1 << INT1));
    sei();
}
//............................................................................
void QV_onIdle(void) {   // called with interrupts DISABLED
    // Put the CPU and peripherals to the low-power mode. You might
    // need to customize the clock management for your application,
    // see the datasheet for your particular AVR MCU.
    SMCR = (0 << SM0) | (1 << SE); // idle mode, adjust to your project
    QV_CPU_SLEEP();  // atomically go to sleep and enable interrupts
}
//............................................................................
void Q_onAssert(char const Q_ROM * const file, int line) {
    // implement the error-handling policy for your application!!!
    display.showNumberDec(114, true);
    for(;;) {}
    QF_INT_DISABLE(); // disable all interrupts
    QF_RESET();  // reset the CPU
}

//============================================================================
// define all AO classes...
/*${AOs::Clock} ............................................................*/
/*${AOs::Clock::SM} ........................................................*/
static QState Clock_initial(Clock * const me) {
    /* ${AOs::Clock::SM::initial} */
    /* Obtain alarm time from EEPROM */

    //me->alarm_hours = EEPROM.read(0);
    //me->alarm_minutes = EEPROM.read(1);
    me->alarm_hours = 0U;
    me->alarm_minutes = 0U;

    /* Make sure it is within bounds */
    if (me->alarm_hours > 23) me->alarm_hours = 23;
    if (me->alarm_minutes > 59) me->alarm_minutes = 59;

    /* Set initial time */
    me->time_hours = 12U;
    me->time_minutes = 0U;
    return Q_TRAN(&Clock_disconnected);
}
/*${AOs::Clock::SM::on} ....................................................*/
static QState Clock_on(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::SET_TIME} */
        case SET_TIME_SIG: {
            uint32_t param = Q_PAR(me);

            uint8_t minutes = param;
            uint8_t hours   = param >> 8;

            me->time_minutes = minutes;
            me->time_hours = hours;
            QACTIVE_POST_X((QActive *)&AO_Clock, 1U, TIME_UPDATED_SIG, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::COMPUTER_DISCONNECTED} */
        case COMPUTER_DISCONNECTED_SIG: {
            status_ = Q_TRAN(&Clock_disconnected);
            break;
        }
        /* ${AOs::Clock::SM::on::TURN_OFF} */
        case TURN_OFF_SIG: {
            status_ = Q_TRAN(&Clock_off);
            break;
        }
        /* ${AOs::Clock::SM::on::ALARM_SET_FROM_COMPUTER} */
        case ALARM_SET_FROM_COMPUTER_SIG: {
            uint32_t param = Q_PAR(me);

            uint8_t minutes = param;
            uint8_t hours   = param >> 8;

            me->alarm_minutes = minutes;
            me->alarm_hours = hours;
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm} .............................................*/
static QState Clock_alarm(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm} */
        case Q_ENTRY_SIG: {
            digitalWrite(LED_ALARM, HIGH);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm} */
        case Q_EXIT_SIG: {
            digitalWrite(LED_ALARM, LOW);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_on);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_minutes} ................................*/
static QState Clock_set_minutes(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_minutes} */
        case Q_EXIT_SIG: {
            BSP_displayTime(me->alarm_hours, me->alarm_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::ROTARY_PUSH} */
        case ROTARY_PUSH_SIG: {
            status_ = Q_TRAN(&Clock_show_alarm);
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::ROTARY_INC} */
        case ROTARY_INC_SIG: {
            me->alarm_minutes++;
            if (me->alarm_minutes > 59) {
                me->alarm_minutes = 0;
            }
            status_ = Q_TRAN(&Clock_minutes_blink_on);
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::ROTARY_DEC} */
        case ROTARY_DEC_SIG: {
            me->alarm_minutes--;
            if (me->alarm_minutes > 59) {
                me->alarm_minutes = 59;
            }
            status_ = Q_TRAN(&Clock_minutes_blink_on);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_alarm);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_on} ..............*/
static QState Clock_minutes_blink_on(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_on} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ALARM_SET_BLINK_TIME, 0U);
            BSP_displayTime(me->alarm_hours, me->alarm_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_on} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_on::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Clock_minutes_blink_off);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_set_minutes);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_off} .............*/
static QState Clock_minutes_blink_off(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_off} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ALARM_SET_BLINK_TIME, 0U);
            BSP_displayHideMinutes();
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_off} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_minutes::minutes_blink_of~::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Clock_minutes_blink_on);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_set_minutes);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::show_alarm} .................................*/
static QState Clock_show_alarm(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::show_alarm} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, BSP_TICKS_PER_SEC*0.5, 0U);

            uint32_t param = (me->alarm_hours << 8) | (me->alarm_minutes);
            /* Save alarm time in EEPROM */
            QACTIVE_POST_X((QActive *)&AO_Computer, 1U, ALARM_SET_SIG, param);
            //EEPROM.write(0, me->alarm_hours);
            //EEPROM.write(1, me->alarm_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::show_alarm} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::show_alarm::ROTARY_PUSH} */
        case ROTARY_PUSH_SIG: {
            status_ = Q_TRAN(&Clock_time);
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::show_alarm::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Clock_time);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_alarm);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_hours} ..................................*/
static QState Clock_set_hours(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_hours} */
        case Q_EXIT_SIG: {
            BSP_displayTime(me->alarm_hours, me->alarm_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::ROTARY_INC} */
        case ROTARY_INC_SIG: {
            me->alarm_hours++;
            if (me->alarm_hours > 23) {
                me->alarm_hours = 0;
            }
            status_ = Q_TRAN(&Clock_hours_blink_on);
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::ROTARY_DEC} */
        case ROTARY_DEC_SIG: {
            me->alarm_hours--;
            if (me->alarm_hours > 23) {
                me->alarm_hours = 23;
            }
            status_ = Q_TRAN(&Clock_hours_blink_on);
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::ROTARY_PUSH} */
        case ROTARY_PUSH_SIG: {
            status_ = Q_TRAN(&Clock_minutes_blink_on);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_alarm);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_hours::hours_blink_on} ..................*/
static QState Clock_hours_blink_on(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_on} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ALARM_SET_BLINK_TIME, 0U);
            BSP_displayTime(me->alarm_hours, me->alarm_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_on} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_on::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Clock_hours_blink_off);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_set_hours);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::alarm::set_hours::hours_blink_off} .................*/
static QState Clock_hours_blink_off(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_off} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ALARM_SET_BLINK_TIME, 0U);
            BSP_displayHideHours();
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_off} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::alarm::set_hours::hours_blink_off::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Clock_hours_blink_on);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_set_hours);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::time} ..............................................*/
static QState Clock_time(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::time} */
        case Q_ENTRY_SIG: {
            digitalWrite(LED_TIME, HIGH);
            BSP_displayTime(me->time_hours, me->time_minutes);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::time} */
        case Q_EXIT_SIG: {
            digitalWrite(LED_TIME, LOW);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::time::ROTARY_PUSH} */
        case ROTARY_PUSH_SIG: {
            status_ = Q_TRAN(&Clock_hours_blink_on);
            break;
        }
        /* ${AOs::Clock::SM::on::time::TIME_UPDATED} */
        case TIME_UPDATED_SIG: {
            BSP_displayTime(me->time_hours, me->time_minutes);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_on);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::on::disconnected} ......................................*/
static QState Clock_disconnected(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::on::disconnected} */
        case Q_ENTRY_SIG: {
            BSP_displayDashes();
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::on::disconnected::TIME_UPDATED} */
        case TIME_UPDATED_SIG: {
            status_ = Q_TRAN(&Clock_time);
            break;
        }
        default: {
            status_ = Q_SUPER(&Clock_on);
            break;
        }
    }
    return status_;
}
/*${AOs::Clock::SM::off} ...................................................*/
static QState Clock_off(Clock * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Clock::SM::off} */
        case Q_ENTRY_SIG: {
            BSP_displayOff();
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Clock::SM::off::TURN_ON} */
        case TURN_ON_SIG: {
            status_ = Q_TRAN(&Clock_disconnected);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

/*${AOs::Computer} .........................................................*/
/*${AOs::Computer::SM} .....................................................*/
static QState Computer_initial(Computer * const me) {
    /* ${AOs::Computer::SM::initial} */
    return Q_TRAN(&Computer_disconnected);
}
/*${AOs::Computer::SM::disconnected} .......................................*/
static QState Computer_disconnected(Computer * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Computer::SM::disconnected} */
        case Q_ENTRY_SIG: {
            QACTIVE_POST((QActive *)&AO_Clock, COMPUTER_DISCONNECTED_SIG, 0U);
            digitalWrite(LED_ACTIVATED, LOW);
            digitalWrite(LED_CONNECTED, LOW);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Computer::SM::disconnected::COMPUTER_ALIVE} */
        case COMPUTER_ALIVE_SIG: {
            status_ = Q_TRAN(&Computer_connected);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::Computer::SM::connected} ..........................................*/
static QState Computer_connected(Computer * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::Computer::SM::connected} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, COMPUTER_SIGNAL_TIMEOUT, 0U);
            //digitalWrite(LED_ALARM, HIGH);
            Serial.println("ALARM?");
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Computer::SM::connected} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            //digitalWrite(LED_ALARM, LOW);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Computer::SM::connected::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&Computer_disconnected);
            break;
        }
        /* ${AOs::Computer::SM::connected::COMPUTER_ALIVE} */
        case COMPUTER_ALIVE_SIG: {
            QActive_armX(&me->super, 0U, COMPUTER_SIGNAL_TIMEOUT, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Computer::SM::connected::ARM_BUTTON_PRESS} */
        case ARM_BUTTON_PRESS_SIG: {
            Serial.println("ARM");
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::Computer::SM::connected::ALARM_SET} */
        case ALARM_SET_SIG: {
            uint32_t param = Q_PAR(me);

            uint8_t minutes = param;
            uint8_t hours   = param >> 8;

            Serial.print("ALARM_SET=");
            if (hours < 10) Serial.print(0);
            Serial.print(hours);
            Serial.print(":");
            if (minutes < 10) Serial.print(0);
            Serial.println(minutes);
            status_ = Q_HANDLED();
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}

/*${AOs::ArmButton} ........................................................*/
/*${AOs::ArmButton::SM} ....................................................*/
static QState ArmButton_initial(ArmButton * const me) {
    /* ${AOs::ArmButton::SM::initial} */
    return Q_TRAN(&ArmButton_unpressed);
}
/*${AOs::ArmButton::SM::unpressed} .........................................*/
static QState ArmButton_unpressed(ArmButton * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::ArmButton::SM::unpressed::ARM_BUTTON_PRESS} */
        case ARM_BUTTON_PRESS_SIG: {
            status_ = Q_TRAN(&ArmButton_pressed);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::ArmButton::SM::pressed} ...........................................*/
static QState ArmButton_pressed(ArmButton * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::ArmButton::SM::pressed} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ARM_BUTTON_TIMEOUT, 0U);
            //digitalWrite(LED_ACTIVATED, HIGH);
            QACTIVE_POST_X((QActive *)&AO_Computer, 1U, ARM_BUTTON_PRESS_SIG, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::ArmButton::SM::pressed} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::ArmButton::SM::pressed::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            /* ${AOs::ArmButton::SM::pressed::Q_TIMEOUT::[stillpressed]} */
            if (((PIND & _BV(DDD5)) == 0)) {
                status_ = Q_TRAN(&ArmButton_waiting_for_release);
            }
            /* ${AOs::ArmButton::SM::pressed::Q_TIMEOUT::[else]} */
            else {
                status_ = Q_TRAN(&ArmButton_unpressed);
            }
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::ArmButton::SM::waiting_for_release} ...............................*/
static QState ArmButton_waiting_for_release(ArmButton * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::ArmButton::SM::waiting_for_rele~::ARM_BUTTON_RELEASE} */
        case ARM_BUTTON_RELEASE_SIG: {
            status_ = Q_TRAN(&ArmButton_after_release);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
/*${AOs::ArmButton::SM::after_release} .....................................*/
static QState ArmButton_after_release(ArmButton * const me) {
    QState status_;
    switch (Q_SIG(me)) {
        /* ${AOs::ArmButton::SM::after_release} */
        case Q_ENTRY_SIG: {
            QActive_armX(&me->super, 0U, ARM_BUTTON_TIMEOUT, 0U);

            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::ArmButton::SM::after_release} */
        case Q_EXIT_SIG: {
            QActive_disarmX(&me->super, 0U);
            status_ = Q_HANDLED();
            break;
        }
        /* ${AOs::ArmButton::SM::after_release::Q_TIMEOUT} */
        case Q_TIMEOUT_SIG: {
            status_ = Q_TRAN(&ArmButton_unpressed);
            break;
        }
        default: {
            status_ = Q_SUPER(&QHsm_top);
            break;
        }
    }
    return status_;
}
