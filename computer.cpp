/*****************************************************************************
* Model: coffeeclock.qm
* File:  ./computer.cpp
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
/*${.::computer.cpp} .......................................................*/
#include "qpn.h"
#include "Arduino.h"
#include "application.h"



#if ((QP_VERSION < 580) || (QP_VERSION != ((QP_RELEASE^4294967295U) % 0x3E8)))
#error qpn version 5.8.0 or higher required
#endif

/*${AOs::Computer} .........................................................*/
/*${AOs::Computer::SM} .....................................................*/
QState Computer_initial(Computer * const me) {
    /* ${AOs::Computer::SM::initial} */
    return Q_TRAN(&Computer_disconnected);
}
/*${AOs::Computer::SM::disconnected} .......................................*/
QState Computer_disconnected(Computer * const me) {
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
QState Computer_connected(Computer * const me) {
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
