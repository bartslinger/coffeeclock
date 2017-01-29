#ifndef BSP_H
#define BSP_H

#define BSP_TICKS_PER_SEC 100 // number of system clock ticks in one second

void BSP_displayTime(uint8_t hours, uint8_t minutes);
void BSP_displayHideHours();
void BSP_displayHideMinutes();
void BSP_displayDashes();
void BSP_displayOff();
void BSP_displayEnable();
void BSP_checkLdrValue(uint16_t analogIn);

#endif // BSP_H
