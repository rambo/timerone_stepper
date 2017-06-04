#ifndef VALUEPARSER_H
#define VALUEPARSER_H

/**
 * Parses ASCII [0-9A-F] hexadecimal to byte value
 */
inline byte ardubus_hex2byte(byte hexchar)
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

inline byte ardubus_hex2byte(byte hexchar0, byte hexchar1)
{
    return (ardubus_hex2byte(hexchar0) << 4) | ardubus_hex2byte(hexchar1);
}

inline int ardubus_hex2int(byte hexchar0, byte hexchar1, byte hexchar2, byte hexchar3)
{
    return ardubus_hex2byte(hexchar0, hexchar1) << 8 | ardubus_hex2byte(hexchar2, hexchar3);
}

inline uint16_t ardubus_hex2uint(byte hexchar0, byte hexchar1, byte hexchar2, byte hexchar3)
{
    return ardubus_hex2byte(hexchar0, hexchar1) << 8 | ardubus_hex2byte(hexchar2, hexchar3);
}

inline int32_t ardubus_hex2long(byte hexchar0, byte hexchar1, byte hexchar2, byte hexchar3, byte hexchar4, byte hexchar5, byte hexchar6, byte hexchar7)
{
    return ardubus_hex2byte(hexchar0, hexchar1) << 24 | ardubus_hex2byte(hexchar2, hexchar3) << 16 | ardubus_hex2byte(hexchar4, hexchar5) << 8 | ardubus_hex2byte(hexchar6, hexchar7);
}


#endif
