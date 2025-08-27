/*
This module is the low-level driver for the EEPROM emulation on the STM32 flash memory
*/

#include "eepromDriver.h"
#include "stm32f0xx.h"

//unlocks the flash memory for the write/erase operations
void flashUnlock(void)
{
    //if already unlocked
    if((FLASH->CR & FLASH_CR_LOCK) == 0)
    {
        return;
    }

    //write the correct unlock sequence
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    while(FLASH->CR & FLASH_CR_LOCK);
}

//lcks the flash memory to prevent writing to it accidentally
void flashLock(void)
{
    FLASH->CR |= FLASH_CR_LOCK;
}

//erases a flash page
void flashErasePage(uint32_t pageAddress)
{
    /*
    steps pulled from STM32F0x1 family reference manual, in order to perform flash memory page erase, the following procedure must be followed:
    1. check that no flash memory operation is ongoing by checking the bsy bit in the flash_cr register
    2. set the per bit in the flash_cr register
    3. program the flash_ar register to select a page to erase
    4. set the strt bit in the flash_cr register
    5. wait for the bsy bit to be reset
    6. check the eop flag in the flash_sr register because it is set when the erase operation has succeeded
    7. clear the eop flag
    */

    //check that no flash mem operation is ongoing
    while (FLASH->SR & FLASH_SR_BSY);
    
    //clear all error flags
    FLASH->SR = FLASH_SR_EOP_Msk | FLASH_SR_WRPERR | FLASH_SR_PGERR;
    
    //set PER bit and page address
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = pageAddress;
    
    //start erase with 10ms timeout
    FLASH->CR |= FLASH_CR_STRT;
    uint32_t timeout = 1000000;
    while ((FLASH->SR & FLASH_SR_BSY) && --timeout);
    
    if (!timeout) 
    {
        printf("\r\nERASE TIMEOUT! SR:0x%08lX", FLASH->SR);
        FLASH->CR &= ~FLASH_CR_PER;
        return;
    }
    
    //verify erase
    if (*(__IO uint32_t*)pageAddress != 0xFFFFFFFF) {
        printf("\r\nERASE VERIFY FAILED @ 0x%08lX", pageAddress);
    }
    
    FLASH->CR &= ~FLASH_CR_PER;
}

void flashWriteHalfword(uint32_t address, uint16_t data) 
{
    //wait for previous operations
    while(FLASH->SR & FLASH_SR_BSY);
    
    //check if write is needed
    if(*(__IO uint16_t*)address == data)
    {
        return;
    }
    
    //check if location is erased
    if(*(__IO uint16_t*)address != 0xFFFF) 
    {
        printf("\r\n WARNING: Write to non-erased location 0x%08lX", address);
    }
    
    FLASH->CR |= FLASH_CR_PG;
    *(__IO uint16_t*)address = data;
}

//reads a 16 bit halfword from the flash memory
uint16_t flashReadHalfword(uint32_t address)
{
    return *(__IO uint16_t*)address;
}

//writes data to the EEPROM
int eepromWrite(uint32_t virtualAddress, const uint8_t* data, uint16_t length)
{
    //calcualte flash address
    uint32_t flashAddress = FLASH_PAGE_62_ADDRESS + virtualAddress;
    
    //check validity of address
    if (flashAddress + length > FLASH_PAGE_63_ADDRESS + FLASH_PAGE_SIZE) 
    {
        return EEPROM_INVALID_ADDRESS;
    }

    //unlock the flash before writing to it
    flashUnlock();

    //write to the flash one halfword at a time
    for (uint16_t i = 0; i < length; i += 2) 
    {
        uint16_t val = (i + 1 < length) ? (data[i + 1] << 8) | data[i] : data[i];
        flashWriteHalfword(flashAddress + i, val);
    }
    flashLock();
    return EEPROM_OK;
}

//reads data from the EEPROM
int eepromRead(uint32_t virtualAddress, uint8_t* buffer, uint16_t length)
{
    //calcualte the flash address
    uint32_t flashAddress = FLASH_PAGE_62_ADDRESS + virtualAddress;
    
    //check for validity of address
    if (flashAddress + length > FLASH_PAGE_63_ADDRESS + FLASH_PAGE_SIZE) 
    {
        return EEPROM_INVALID_ADDRESS;
    }

    //read one halfword at a time
    for (uint16_t i = 0; i < length; i += 2) 
    {
        uint16_t val = flashReadHalfword(flashAddress + i);
        buffer[i] = val & 0xFF;
        if (i + 1 < length) 
        {
            buffer[i + 1] = (val >> 8) & 0xFF;
        }
    }
    
    return EEPROM_OK;
}

