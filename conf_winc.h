#ifndef CONF_WINC_H_INCLUDED
#define CONF_WINC_H_INCLUDED

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define CONF_WINC_PIN_CHIP_ENABLE 7
#define CONF_WINC_PIN_RESET 6
#define CONF_WINC_PIN_WAKE 8

#define CONF_WINC_SPI_PORT spi1
#define CONF_WINC_SPI_CS_PIN 13
#define CONF_WINC_SPI_MISO_PIN 12
#define CONF_WINC_SPI_MOSI_PIN 11
#define CONF_WINC_SPI_SCK_PIN 10
#define CONF_WINC_SPI_INT_PIN 9

// </h>

// <h> WINC Pico Specific Configuration
// <q> CONF_WINC_USE_PICO
// <i> Use Pico specific BSP and Bus Wrapper
#define CONF_WINC_USE_PICO
// </h>

// <h> WINC Debug Configuration
// <q> CONF_WINC_DEBUG
// <i> Enable WINC debug prints

#define CONF_WINC_DEBUG 1
#define M2M_LOG_LEVEL 4
#define CONF_WINC_PRINTF printf

#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H_INCLUDED */
