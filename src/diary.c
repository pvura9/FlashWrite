/*
This module manages the diary entries in the EEPROM and handles storage and retrieval operations.
*/
#include <stdio.h>
#include "stm32f0xx.h" 
#include "diary.h"
#include "eepromDriver.h"
#include "crypto.h"
#include "rtc.h"
#include <string.h>
#include "stm32f0xx.h"

#define INDEX_TABLE_ADDRESS FLASH_PAGE_62_ADDRESS
#define CONTENT_START_ADDRESS (FLASH_PAGE_62_ADDRESS + 0x200)
#define DEBUG_SEARCH 1

uint32_t findNextFreeAddress() 
{
    int count = getEntryCount();
    uint32_t highestUsed = CONTENT_START_ADDRESS;
    
    //find the highest used address
    for(int i = 0; i < count; i++) 
    {
        DiaryEntryIndex meta;
        uint32_t metaAddress = FLASH_PAGE_62_ADDRESS + i * sizeof(DiaryEntryIndex);
        eepromRead(metaAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)&meta, sizeof(meta));
        
        if(meta.flashAddress != 0xFFFFFFFF && meta.flashAddress + meta.length > highestUsed) 
        {
            highestUsed = meta.flashAddress + meta.length;
        }
    }
    
    //align to the next halfword bound
    return (highestUsed + 1) & ~1;
}

int storeDiaryEntry(const char* tag, const uint8_t* content, uint16_t len, uint8_t encrypt)
{
    //find the next available space
    uint32_t contentAddress;
    contentAddress = findNextFreeAddress();
    
    //verify the space
    if(contentAddress + len > FLASH_PAGE_63_ADDRESS + FLASH_PAGE_SIZE) 
    {
        //error if not enough space
        printf("\r\nERROR: Insufficient flash space!");
        return -1;
    }

    //prepare the metadata
    DiaryEntryIndex meta = 
    {
        .flashAddress = contentAddress,
        .length = len,
        .timestamp = rtcGetTimestamp()
    };
    strncpy(meta.tag, tag, MAX_TAG_LENGTH - 1);
    meta.tag[MAX_TAG_LENGTH-1] = '\0';
    printf("\r\nWriting content to 0x%08lX...", contentAddress);
    
    //write the content
    flashUnlock();
    
    //check if have to erase the content page
    uint32_t contentPage = (contentAddress >= FLASH_PAGE_63_ADDRESS) ? FLASH_PAGE_63_ADDRESS : FLASH_PAGE_62_ADDRESS;

    //erase the page if address and page match       
    if(contentAddress == contentPage) 
    {
        flashErasePage(contentPage);
    }

    uint32_t startTime = rtcGetTimestamp();
    uint32_t timeoutMs = 1000;
    
    for(uint16_t i = 0; i < len; i += 2) 
    {
        uint16_t val = (i + 1 < len) ? (content[i + 1] << 8) | content[i] : content[i];
        
        flashWriteHalfword(contentAddress + i, val);
        
        while(!(FLASH->SR & FLASH_SR_EOP_Msk)) 
        {
            if((rtcGetTimestamp() - startTime) > timeoutMs) 
            {
                printf("\r\nERROR: Flash write timeout!");
                flashLock();
                return -1;
            }
        }
        FLASH->SR = FLASH_SR_EOP_Msk;
    }

    //write the prepared metadata
    uint32_t metaAddress = FLASH_PAGE_62_ADDRESS + getEntryCount() * sizeof(DiaryEntryIndex);
    
    //check if need to erase index page
    if(metaAddress >= FLASH_PAGE_63_ADDRESS) 
    {
        flashErasePage(FLASH_PAGE_63_ADDRESS);
    }

    uint8_t *metaPointer = (uint8_t*)&meta;
    for(uint16_t i = 0; i < sizeof(meta); i += 2) 
    {
        uint16_t val = (i+1 < sizeof(meta)) ? (metaPointer[i+1] << 8) | metaPointer[i] : metaPointer[i];
        
        flashWriteHalfword(metaAddress + i, val);
        startTime = rtcGetTimestamp();
        while(!(FLASH->SR & FLASH_SR_EOP_Msk)) 
        {
            if((rtcGetTimestamp() - startTime) > timeoutMs) 
            {
                printf("\r\nERROR: Metadata write timeout!");
                flashLock();
                return -1;
            }
        }
        FLASH->SR = FLASH_SR_EOP_Msk;
    }
    
    //lock the flash before returning
    flashLock();
    return 0;
}

int retrieveDiaryEntry(uint16_t index, char* outputBuffer, uint8_t decrypt) 
{
    DiaryEntryIndex meta;
    uint32_t metaAddress = FLASH_PAGE_62_ADDRESS + index * sizeof(DiaryEntryIndex);
    
    //read the metadata for retrieval
    if(eepromRead(metaAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)&meta, sizeof(meta)) != EEPROM_OK) 
    {
        printf("\r\nError: Invalid entry index");
        return -1;
    }
    
    //verify if the entry exists
    if(meta.flashAddress == 0xFFFFFFFF) 
    {
        printf("\r\nError: Entry has been deleted");
        return -1;
    }
    
    //read the content if entry exists
    if(eepromRead(meta.flashAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)outputBuffer, meta.length) != EEPROM_OK) 
    {
        return -1;
    }
    
    //make sure null termination
    outputBuffer[meta.length] = '\0';
    
    //decryption
    if(decrypt) {
        xorDecrypt((uint8_t*)outputBuffer, meta.length, ENCRYPTION_KEY);
    }
    
    //return the length of metadata
    return meta.length;
}

int getEntryCount(void) 
{
    uint32_t addr = INDEX_TABLE_ADDRESS;
    DiaryEntryIndex meta;
    int count = 0;
    
    while (addr < CONTENT_START_ADDRESS) 
    {
        if(eepromRead(addr - FLASH_PAGE_62_ADDRESS, (uint8_t*)&meta, sizeof(meta)) != EEPROM_OK) 
        {
            break;
        }
        if(meta.flashAddress == 0xFFFFFFFF) 
        {
            break;
        }
        count++;
        addr += sizeof(DiaryEntryIndex);
    }
    
    return count;
}

int findEntryByTag(const char* tag, DiaryEntryIndex* result) 
{
    int count = getEntryCount();
    //debug search print for testing
    #if DEBUG_SEARCH
    printf("\r\nSEARCH DEBUG: Looking for '%s' (%d entries)", tag, count);
    #endif

    char cleanTag[MAX_TAG_LENGTH];
    strncpy(cleanTag, tag, MAX_TAG_LENGTH-1);
    cleanTag[MAX_TAG_LENGTH-1] = '\0';

    for (int i = 0; i < count; i++) 
    {
        uint32_t metaAddress = INDEX_TABLE_ADDRESS + i * sizeof(DiaryEntryIndex);
        if (eepromRead(metaAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)result, sizeof(DiaryEntryIndex)) != EEPROM_OK) 
        {
            return -1;
        }


        #if DEBUG_SEARCH
        printf("\r\n Entry %d: tag = '%s' address = 0x%08lX", i, result->tag, result->flashAddress);

        printf("\r\n Raw tag bytes: ");
        for(int j = 0; j < MAX_TAG_LENGTH; j++) 
        {
            printf("%02X ", result->tag[j] & 0xFF);
        }
        #endif

       if(strncmp(result->tag, cleanTag, MAX_TAG_LENGTH) == 0) 
       {
            #if DEBUG_SEARCH
            printf("\r\n  MATCH FOUND!");
            #endif
            return 0;
        }
    }
    return -1;
}