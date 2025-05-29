#include "eepromDriver.h"
#include <string.h>

void test_eeprom_driver(void)
{
    uint8_t test_data[] = "Hello Project!";
    uint8_t read_buffer[sizeof(test_data)] = {0};

    int status = eeprom_write(0, test_data, sizeof(test_data));
    if(status != EEPROM_OK)
    {
        //error
        return;
    }
    status = eeprom_read(0, read_buffer, sizeof(test_data));
    if(status != EEPROM_OK)
    {
        //error
        return;
    }

    if(memcmp(test_data, read_buffer, sizeof(test_data)))
    {
        return;
    }

    //test passed
}