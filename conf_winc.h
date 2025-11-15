#ifndef CONF_WINC_H_INCLUDED
#define CONF_WINC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// <h> WINC SPI Bus Configuration

// <q> CONF_WINC_USE_SPI
// <i> Use SPI bus for WINC communication
#define CONF_WINC_USE_SPI

#define NM_EDGE_INTERRUPT   (1)
//#define NM_LEVEL_INTERRUPT  (0)

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
#define CONF_WINC_DEBUG 4

// <h> M2M_LOG_LEVEL
// <i> Set WINC debug log level (M2M_LOG_NONE, M2M_LOG_ERROR, M2M_LOG_INFO, M2M_LOG_REQ, M2M_LOG_DBG)
#define M2M_LOG_LEVEL M2M_LOG_DBG

// <h> CONF_WINC_PRINTF
// <i> Define the printf function to use for WINC debug prints
#define CONF_WINC_PRINTF printf
// </h>

#ifdef __cplusplus
}
#endif

#endif /* CONF_WINC_H_INCLUDED */
