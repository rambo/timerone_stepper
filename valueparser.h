#ifndef VALUEPARSER_H
#define VALUEPARSER_H

/**
 * Parses ASCII [0-9A-F] hexadecimal to byte value
 */
uint8_t ardubus_hex2nibble(char hexchar)
{
    if (   0x40 < hexchar
        && hexchar < 0x47) // A-F
    {
        return (hexchar - 0x41) + 10;
    }
    if (   0x2f < hexchar
        && hexchar < 0x3a) // 0-9
    {
        return (hexchar - 0x30);
    }
    return 0x0; // Failure.

}

uint8_t ardubus_hex2uint8_t(char hexchar0, char hexchar1)
{
    return ((uint8_t)ardubus_hex2nibble(hexchar0) << 4) | ardubus_hex2nibble(hexchar1);
}

int ardubus_hex2int(char hexchar0, char hexchar1, char hexchar2, char hexchar3)
{
    return ((int16_t)ardubus_hex2uint8_t(hexchar0, hexchar1) << 8) | ardubus_hex2uint8_t(hexchar2, hexchar3);
}

uint16_t ardubus_hex2uint(char hexchar0, char hexchar1, char hexchar2, char hexchar3)
{
    return ((uint16_t)ardubus_hex2uint8_t(hexchar0, hexchar1) << 8) | ardubus_hex2uint8_t(hexchar2, hexchar3);
}

int32_t ardubus_hex2long(char hexchar0, char hexchar1, char hexchar2, char hexchar3, char hexchar4, char hexchar5, char hexchar6, char hexchar7)
{
    return ((int32_t)ardubus_hex2uint8_t(hexchar0, hexchar1) << 24) | ((int32_t)ardubus_hex2uint8_t(hexchar2, hexchar3) << 16) | ((uint16_t)ardubus_hex2uint8_t(hexchar4, hexchar5) << 8) | ardubus_hex2uint8_t(hexchar6, hexchar7);
}


#endif
