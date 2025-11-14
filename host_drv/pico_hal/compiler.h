#ifndef COMPILER_H_INCLUDED
#define COMPILER_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef uint8_t      uint8;
typedef uint16_t     uint16;
typedef uint32_t     uint32;
typedef int8_t       sint8;
typedef int16_t      sint16;
typedef int32_t      sint32;

#define M2M_SUCCESS         ((sint8)0)
#define M2M_ERR_SEND        ((sint8)-1)
#define M2M_ERR_RCV         ((sint8)-2)
#define M2M_ERR_MEM_ALLOC   ((sint8)-3)
#define M2M_ERR_TIMEOUT     ((sint8)-4)
#define M2M_ERR_INIT        ((sint8)-5)
#define M2M_ERR_BUS_FAIL    ((sint8)-6)
#define M2M_NOT_YET         ((sint8)-7)
#define M2M_ERR_FIRMWARE    ((sint8)-8)

#define M2M_NO_POWERSAVE    M2M_NO_PS

#define HIGH 1
#define LOW 0

#define min(a,b) (((a) < (b)) ? (a) : (b))

#endif /* COMPILER_H_INCLUDED */
