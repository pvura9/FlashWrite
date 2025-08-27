/*
This module implements simple XOR encryption/decryption logic
*/

#include "crypto.h"
void xorEncrypt(uint8_t* data, uint16_t length, uint8_t key) 
{
    for(uint16_t i = 0; i < length; i++) 
    {
        //xor each bit
        data[i] ^= key;
        //rotate the key right by 3 bits
        key = (key >> 3) | (key << 5);
    }
}

//the decryption is the exact same as the ecryption process because the XOR operation is symmetric
void xorDecrypt(uint8_t* data, uint16_t length, uint8_t key) 
{
    xorEncrypt(data, length, key); 
}