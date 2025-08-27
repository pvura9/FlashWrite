#ifndef RTC_H
#define RTC_H
#include <stdint.h>

void rtcInit(void);
uint32_t rtcGetTimestamp(void);
void rtcSetTimestamp(uint32_t timestamp);

#endif