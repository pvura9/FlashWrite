/*
This module handles the user commands via UART I/O
*/

#include "stm32f0xx.h"
#include "serial.h"
#include "diary.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "eepromDriver.h"

//ignore all newlines
static void flushInput(void)
{
    while(getchar() != '\n')
    {
        //ntng
    }
}

void handleWriteCommand(void) 
{
    char tag[MAX_TAG_LENGTH];
    char content[MAX_CONTENT_LENGTH];
    char c;
    int idx = 0;
    
    printf("\r\nEnter tag (max %d chars): ", MAX_TAG_LENGTH-1);
    fgets(tag, MAX_TAG_LENGTH, stdin);
    tag[strcspn(tag, "\n")] = '\0';
    
    printf("\r\nEnter content (end with #): ");
    while((c = getchar()) != '#' && idx < MAX_CONTENT_LENGTH-1) 
    {
        content[idx++] = c;
    }
    content[idx] = '\0';
    
    //encrypt before storing
    xorEncrypt((uint8_t*)content, idx, ENCRYPTION_KEY);
    
    if(storeDiaryEntry(tag, (uint8_t*)content, idx + 1, 0) == 0) 
    {
        printf("\r\nEntry saved successfully!\r\n");
    } 
    else 
    {
        printf("\r\nFailed to save entry!\r\n");
    }
}

void handleLogoutCommand(void) 
{
    printf("\r\nYou've been logged out. Goodbye!\r\n");
    //soft reset the uC upon logout
    NVIC_SystemReset();
}

void handleSearchCommand(const char* tag) 
{
    DiaryEntryIndex meta;
    printf("\r\nSearching for '%s'...", tag);
    
    if(findEntryByTag(tag, &meta) == 0) 
    {
        printf("\r\n=== Found Entry ===");
        printf("\r\nTag: %s", meta.tag);
        printf("\r\nTimestamp: %lu", meta.timestamp);
        printf("\r\nAddress: 0x%08lX", meta.flashAddress);
        printf("\r\nSize: %d bytes\r\n", meta.length);
    } 
    else 
    {
        printf("\r\nNo matching entries found");
    }
}

void handleReadCommand(uint16_t index) 
{
    //add 1 for null term
    char content[MAX_CONTENT_LENGTH + 1];
    
    printf("\r\nReading entry %d...", index);
    
    //1 = decrypt
    int len = retrieveDiaryEntry(index, content, 1);
    
    if(len > 0) 
    {
        printf("\r\n=== Entry %d ===", index);
        printf("\r\nContent: %s", content);
        printf("\r\n================\r\n");
    } 
    else 
    {
        printf("\r\nFailed to read entry");
    }
}

void handleDeleteCommand(uint16_t index) 
{
    DiaryEntryIndex meta;
    uint32_t metaAddress = FLASH_PAGE_62_ADDRESS + index * sizeof(DiaryEntryIndex);
    
    //read the existing metadata
    if(eepromRead(metaAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)&meta, sizeof(meta)) != EEPROM_OK) 
    {
        printf("\r\nError: Invalid entry index");
        return;
    }
    
    //verify entry exists
    if(meta.flashAddress == 0xFFFFFFFF) 
    {
        printf("\r\nEntry %d is already deleted", index);
        return;
    }
    
    //prepare deletion marker
    DiaryEntryIndex deletedMarker;
    memset(&deletedMarker, 0xFF, sizeof(DiaryEntryIndex));
    
    //perform deletioon
    flashUnlock();
    
    //erase the entire page
    flashErasePage(FLASH_PAGE_62_ADDRESS);
    
    //rewrite all entries except the deleted one
    int count = getEntryCount();
    for(int i = 0; i < count; i++) 
    {
        uint32_t currentAddr = FLASH_PAGE_62_ADDRESS + i * sizeof(DiaryEntryIndex);
        
        //skip the entry deleting
        if(i == index) continue;
        
        //read existing entry
        DiaryEntryIndex currentEntry;
        eepromRead(currentAddr - FLASH_PAGE_62_ADDRESS, (uint8_t*)&currentEntry, sizeof(currentEntry));
        
        //rewrite it
        uint8_t *entryBytes = (uint8_t*)&currentEntry;
        for(uint16_t j = 0; j < sizeof(DiaryEntryIndex); j += 2) 
        {
            uint16_t val = (j+1 < sizeof(DiaryEntryIndex)) ?(entryBytes[j+1] << 8) | entryBytes[j] : entryBytes[j];

            flashWriteHalfword(currentAddr + j, val);
            while(!(FLASH->SR & FLASH_SR_EOP_Msk));
            FLASH->SR = FLASH_SR_EOP_Msk;
        }
    }
    
    //write the deleted marker at the end
    uint32_t newAddr = FLASH_PAGE_62_ADDRESS + count * sizeof(DiaryEntryIndex);
    uint8_t *markerBytes = (uint8_t*)&deletedMarker;
    for(uint16_t j = 0; j < sizeof(DiaryEntryIndex); j += 2) 
    {
        uint16_t val = (j + 1 < sizeof(DiaryEntryIndex)) ? (markerBytes[j+1] << 8) | markerBytes[j] : markerBytes[j];

        flashWriteHalfword(newAddr + j, val);
        while(!(FLASH->SR & FLASH_SR_EOP_Msk));
        FLASH->SR = FLASH_SR_EOP_Msk;
    }
    
    //lock flash and display proper statement
    flashLock();
    printf("\r\nEntry %d deleted successfully!", index);
}

void handleListCommand(void) 
{
    int count = getEntryCount();
    if(count == 0) 
    {
        printf("\r\nNo entries found");
        return;
    }
    
    printf("\r\n=== Entries (%d) ===", count);
    
    for(int i = 0; i < count; i++) 
    {
        DiaryEntryIndex meta;
        uint32_t metaAddress = FLASH_PAGE_62_ADDRESS + (i * sizeof(DiaryEntryIndex));
        
        if(eepromRead(metaAddress - FLASH_PAGE_62_ADDRESS, (uint8_t*)&meta, sizeof(meta)) == EEPROM_OK) 
        {
            //show only show valid entries instead of deleted ones also
            if(meta.flashAddress != 0xFFFFFFFF) 
            {
                printf("\r\n%2d: [%s] (Time: %lu, Size: %d bytes)",  i, meta.tag, meta.timestamp, meta.length);
            }
        }
    }
}

//parse the input commands
void parseCommand(const char* input) 
{
    //ignore nulls and newlines
    if(input[0] == '\0' || input[0] == '\n') 
    {
        return;
    }
    if(strncmp(input, "write", 5) == 0) 
    {
        handleWriteCommand();
    }
    else if(strncmp(input, "search ", 7) == 0) 
    {
        handleSearchCommand(input + 7);
    }
    else if(strncmp(input, "read ", 5) == 0) 
    {
        handleReadCommand(atoi(input + 5));
    }
    else if(strncmp(input, "logout", 6) == 0) 
    {
        handleLogoutCommand();
    }
    else 
    {
        printf("\r\nUnknown command");
    }
    flushInput();
}