/*
For the EEPROM emulation, I will use 2KB (2048) flash pages (62 and 63) on the STM32
(a "flash page" is a storage unit within the memory that can be written to and erased as a single entity)

Page 62: 0x800F800 - 0x0800FFFF (134281216 - 134283263)
Page 63: 0x0800FC00 - 0x0800FFFF (134282240 - 134283263)

*/
#ifndef EEPROM_DRIVER_H
#define EEPROM_DRIVER_H
#include <stdint.h>

//these are the flash page definitions
#define FLASH_BASE_ADDRESS 0x08000000
#define FLASH_PAGE_SIZE 2048
#define FLASH_PAGE_62_ADDRESS 0x0800F800
#define FLASH_PAGE_63_ADDRESS 0x0800FC00

//EEPROM emulation parameters
#define EEPROM_START_ADDRESS FLASH_PAGE_62_ADDRESS
#define EEPROM_END_ADDRESS FLASH_PAGE_63_ADDRESS + FLASH_PAGE_SIZE
#define EEPROM_SIZE 2*FLASH_PAGE_SIZE

//Custom codes
#define EEPROM_OK              0
#define EEPROM_ERROR           1
#define EEPROM_INVALID_ADDRESS 2
#define EEPROM_WRITE_FAILED    3

//Function declarations
void flashUnlock(void);
void flashLock(void);
void flashErasePage(uint32_t);
void flashWriteHalfword(uint32_t, uint16_t);
uint16_t flashReadHalfword(uint32_t);
int eepromWrite(uint32_t, uint8_t*, uint16_t);
int eepromRead(uint32_t, uint8_t*, uint16_t);

#endif