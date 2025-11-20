#ifndef WINC1500_REGISTERS_H
#define WINC1500_REGISTERS_H

// Register Definitions
#define CHIPID                  0x0000
#define VERSION                 0x0004
#define STATUS                  0x0008
#define CONTROL                 0x000C
#define INT_STATUS              0x0010
#define INT_ENABLE              0x0014
#define WIFI_STATUS             0x1000
#define WIFI_CONTROL            0x1004
#define WIFI_TX_DATA            0x1008
#define WIFI_RX_DATA            0x100C
#define SOCKET_STATUS           0x2000
#define SOCKET_CONTROL          0x2004
#define SOCKET_TX_DATA          0x2008
#define SOCKET_RX_DATA          0x200C
#define NMI_GLB_RESET_0         0x1400
#define NMI_PIN_MUX_0           0x1408
#define NMI_INTR_ENABLE         0x1A00
#define NMI_STATE_REG           0x108C
#define BOOTROM_REG             0x3000
#define WIFI_HOST_RCV_CTRL_4    0x1504
#define WIFI_HOST_RCV_CTRL_1    0x1084
#define WIFI_HOST_RCV_CTRL_2    0x1078
#define WIFI_HOST_RCV_CTRL_3    0x106C
#define WIFI_HOST_RCV_CTRL_5    0x1088
#define NMI_SPI_REG_BASE        0xE800
#define NMI_SPI_PROTOCOL_CONFIG 0xE824  // Bits 2 and 3 control CRC
#define NMI_SPI_CTL             (NMI_SPI_REG_BASE)
#define NMI_SPI_MASTER_DMA_ADDR (NMI_SPI_REG_BASE+0x4)
#define NMI_SPI_MASTER_DMA_COUNT (NMI_SPI_REG_BASE+0x8)
#define NMI_SPI_SLAVE_DMA_ADDR  (NMI_SPI_REG_BASE+0xc)
#define NMI_SPI_SLAVE_DMA_COUNT (NMI_SPI_REG_BASE+0x10)
#define NMI_SPI_TX_MODE         (NMI_SPI_REG_BASE+0x20)
#define NMI_SPI_INTR_CTL        (NMI_SPI_REG_BASE+0x2c)
#define WAKE_CLK_REG            0x1
#define CLOCKS_EN_REG           0xF
#define WIFI_HOST_RCV_CTRL_0    0x1070
#define NMI_REV_REG             0x207AC

#endif // WINC1500_REGISTERS_H
