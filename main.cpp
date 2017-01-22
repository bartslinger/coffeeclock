#include "qpn.h"
#include "Arduino.h"
#include <TM1637Display.h>

#include "application.h"

Q_DEFINE_THIS_MODULE("coffeeclock")

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

struct serialBuffer {
    char data[SERIAL_BUFFER_SIZE];
    uint8_t idx;
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
    sBuf.idx = 0;
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
