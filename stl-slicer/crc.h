
#ifndef CRC_H_
#define CRC_H_

#include <stdint.h>
#ifndef FALSE
#define FALSE	0
#define TRUE	!FALSE
#endif
/*
 * Select the CRC standard from the list that follows.
 */
#define CRC_CCITT

// 19 SEP 2018 EDIT - Changing CRC type to CCITT because it's much faster to calculate

#if defined(CRC_CCITT)

typedef uint16_t  crc;

#define CRC_NAME			"CRC-CCITT"
#define POLYNOMIAL			0x1021
#define INITIAL_REMAINDER	0xFFFF
#define FINAL_XOR_VALUE		0x0000
#define REFLECT_DATA		FALSE
#define REFLECT_REMAINDER	FALSE
#define CHECK_VALUE			0x29B1

#elif defined(CRC16)

typedef unsigned short  crc;

#define CRC_NAME			"CRC-16"
#define POLYNOMIAL			0x8005
#define INITIAL_REMAINDER	0x0000
#define FINAL_XOR_VALUE		0x0000
#define REFLECT_DATA		TRUE
#define REFLECT_REMAINDER	TRUE
#define CHECK_VALUE			0xBB3D

#elif defined(CRC32)

typedef unsigned long  crc;

#define CRC_NAME			"CRC-32"
#define POLYNOMIAL			0x04C11DB7
#define INITIAL_REMAINDER	0xFFFFFFFF
#define FINAL_XOR_VALUE		0xFFFFFFFF
#define REFLECT_DATA		TRUE
#define REFLECT_REMAINDER	TRUE
#define CHECK_VALUE			0xCBF43926

#else

#error "One of CRC_CCITT, CRC16, or CRC32 must be #define'd."

#endif


void  crcInit(void);
crc   crcSlow(unsigned char const *message, int nBytes);
crc   crcFast(unsigned char const *message, int nBytes);

#endif
