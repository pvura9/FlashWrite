#ifndef SERIAL_H
#define SERIAL_H
#include <stdint.h>

void handleWriteCommand(void);
void handleSearchCommand(const char* tag);
void handleReadCommand(uint16_t index);
void parseCommand(const char* input);
void handleDeleteCommand(uint16_t index);
void handleListCommand(void);
void handleLogoutCommand(void);
#endif