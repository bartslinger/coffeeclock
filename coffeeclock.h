#ifndef COFFEECLOCK_H
#define COFFEECLOCK_H

#include "qpn.h"
#include "clock.h"
#include "computer.h"
#include "arm_button.h"

class CoffeeClock
{
public:
    CoffeeClock();

    // Instantiate Active Objects and event queues
    Clock         AO_Clock;
    static QEvt   l_clockQueue[10];

    Computer      AO_Computer;
    static QEvt   l_computerQueue[10];

    ArmButton     AO_ArmButton;
    static QEvt   l_armButtonQueue[2];
};

#endif // COFFEECLOCK_H
