#ifndef CRYPTO_H
#define CRYPTO_H
#include <stdint.h>

void xorEncrypt(uint8_t*, uint16_t, uint8_t);
void xorDecrypt(uint8_t*, uint16_t, uint8_t);

#endif