#ifndef _COMPILER_H_
#define _COMPILER_H_

#define UNUSED(v) (void)v
#define DEPRECATED
#define WEAK __attribute__((weak))

#define COMPILER_PACK_SET(n) __attribute__((packed, aligned(n)))
#define COMPILER_PACK_RESET()

#define HIGH 1
#define LOW 0

#endif /* _COMPILER_H_ */
