#ifndef DIARY_H
#define DIARY_H

#include <stdint.h>
#include <stdio.h>
#include "stm32f0xx.h" 

#define MAX_TAG_LENGTH 16
#define MAX_CONTENT_LENGTH 128
#define MAX_ENTRIES 50
#define ENCRYPTION_KEY 0x55


#define INDEX_TABLE_ADDRESS FLASH_PAGE_62_ADDRESS
#define CONTENT_START_ADDRESS (FLASH_PAGE_62_ADDRESS + 0x200)

typedef struct 
{
    uint32_t flashAddress;
    uint16_t length;
    char tag[MAX_TAG_LENGTH];
    uint32_t timestamp;
} DiaryEntryIndex;

int addEntryIndex(const DiaryEntryIndex*);
int getAllEntryIndices(DiaryEntryIndex*, uint16_t);
int findEntryByTag(const char*, DiaryEntryIndex*);
int storeDiaryEntry(const char* tag, const uint8_t* content, uint16_t length, uint8_t encrypt);
int loadDiaryEntry(uint32_t, uint16_t, uint8_t*);
int getEntryCount(void);
int retrieveDiaryEntry(uint16_t index, char* outputBuffer, uint8_t decrypt);
uint32_t findNextFreeAddress();

#endif