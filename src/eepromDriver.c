#include "eepromDriver.h"
#include "stm32f0xx.h" //cmsis header file for stm32

//this function is responsible for unlocking the flash memory for the write and erase operations
void flashUnlock(void)
{
    //if already unlocked
    if((FLASH->CR & FLASH_CR_LOCK) != 1)
    {
        return;
    }

    //write the correct unlock sequence
    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    while(FLASH->CR & FLASH_CR_LOCK)
    {
        //do ntng
    }

}

//this function locks the flash memory to prevent writing to it accidentally
void flashLock(void)
{
    //set the right bits to lock the flash mem
    FLASH->CR |= FLASH_CR_LOCK;
}

//this function erases a flash page
void flashErasePage(uint32_t pageAddress)
{
    /*
    according to the STM32F0x1 family reference manual, in order to perform flash memory page erase, the following procedure must be followed:
    1. check that no flash memory operation is ongoing by checking the bsy bit in the flash_cr register
    2. set the per bit in the flash_cr register
    3. program the flash_ar register to select a page to erase
    4. set the strt bit in the flash_cr register
    5. wait for the bsy bit to be reset
    6. check the eop flag in the flash_sr register because it is set when the erase operation has succeeded
    7. clear the eop flag
    */

    //if it is already page aligned, do ntng
    if((pageAddress & (FLASH_PAGE_SIZE - 1)) != 0)
    {
        return;
    }

    //the BSY bit indicates whether there is an ongoing operation
    while(FLASH->SR & FLASH_SR_BSY)
    {
        //ntng;
    }

    //setting the appropriate bits in the CR and AR registers and start the erasing operation
    FLASH->CR |= FLASH_CR_PER;
    FLASH->AR = pageAddress;
    FLASH->CR |= FLASH_CR_STRT;

    while(FLASH->SR & FLASH_SR_BSY)
    {
        //do ntng
    }

    // Clear EOP flag if it is set
    if (FLASH->SR & FLASH_SR_EOP) 
    {
        FLASH->SR = FLASH_SR_EOP;
    }
    
    // Clear PER bit
    FLASH->CR &= ~FLASH_CR_PER;

}

//this function writes a 16 bit halfword to the flash memory
void flashWriteHalfword(uint32_t address, uint16_t data)
{
    //the BSY bit indicates whether there is an ongoing operation
    while(FLASH->SR & FLASH_SR_BSY)
    {
        //ntng;
    }

    //set the pg bit
    FLASH->CR |= FLASH_CR_PG;

    //perform the write
    /*
    __IO is a macro defined in CMSIS - tells compiler that this memory can change unexpectedly so don't optimize access to that location

    uint16_t* casts the address into a pointer to a 16-bit unsigned integer because flash writes on the stm32 must be half-word aligned


    *(...) = data dereferences the pointer and writes the data to the flash address*/
    *(__IO uint16_t*)address = data;

    //the BSY bit indicates whether there is an ongoing operation
    while(FLASH->SR & FLASH_SR_BSY)
    {
        //ntng;
    }

    // Clear EOP flag if it is set
    if (FLASH->SR & FLASH_SR_EOP) 
    {
        FLASH->SR = FLASH_SR_EOP;
    }

    //clear the pg bit
    FLASH->CR &= ~FLASH_CR_PG;
}

//this function reads a 16 bit halfword from the flash memory
uint16_t flashReadHalfword(uint32_t address)
{
    return *(__IO uint16_t*)address;
}

//this function writes data to the EEPROM
int eepromWrite(uint32_t virtualAddress, uint8_t* data, uint16_t length)
{
    //check if the address is within the acceptable bounds
    if(virtualAddress + length > EEPROM_SIZE)
    {
        return EEPROM_INVALID_ADDRESS;
    }
    //calculate the flash address
    uint32_t flashAddress = EEPROM_START_ADDRESS + virtualAddress;

    //the flash can only be written to when it has been erased so must check whether or not writing to an erased location
    for(uint16_t i = 0; i < length; i=i+2)
    {
        uint16_t existing = flashReadHalfword(flashAddress + i);
        //has not been erased
        if(existing != 0xFFFF)
        {
            return EEPROM_WRITE_FAILED;
        }
    } 

    flashUnlock();

    //write the data in proper sized chunks
    for(uint16_t i = 0; i < length; i=i+2) //process in installments of 2 bytes
    {
        uint16_t value;
        //when 2 bytes available, combine the 2; 8 bit pieces into one 16 bit value
        if(i+1 < length)
        {
            value = (data[i+1] << 8) | data[i];
        }
        //when only 1 byte is remaining, just use that single byte. leading bits will be filled in with 0s in this case
        else
        {
            value = data[i];
        }

        //write that found value to flash at specified address
        flashWriteHalfword(flashAddress + i, value);

        //invalid write
        if(flashReadHalfword(flashAddress + i) != value)
        {
            flashLock();
            return EEPROM_WRITE_FAILED;
        }
    }
    
    //done
    flashLock();
    return EEPROM_OK;
}

//this function reads data from the EEPROM
int eepromRead(uint32_t virtualAddress, uint8_t* buffer, uint16_t length)
{
    if(virtualAddress + length > EEPROM_SIZE)
    {
        return EEPROM_INVALID_ADDRESS;
    }

    uint32_t flashAddress = EEPROM_START_ADDRESS + virtualAddress;
    for(uint16_t i = 0; i < length; i=i+2) //process in installments of 2 bytes
    {
        //read that specific halfword
        uint16_t value = flashReadHalfword(flashAddress + i);
        if(i+1 < length)
        {
            buffer[i] = value & 0xFF;
            buffer[i+1] = (value >> 8) & 0xFF;
        }
        else
        {
            buffer[i] = value & 0xFF;
        }
    }

    //done
    return EEPROM_OK;
}

