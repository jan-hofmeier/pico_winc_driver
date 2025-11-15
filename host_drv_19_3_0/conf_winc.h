#ifndef CONF_WINC_H_INCLUDED
#define CONF_WINC_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

// <h> WINC SPI Bus Configuration

// <q> CONF_WINC_USE_SPI
// <i> Use SPI bus for WINC communication
#define CONF_WINC_USE_SPI

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