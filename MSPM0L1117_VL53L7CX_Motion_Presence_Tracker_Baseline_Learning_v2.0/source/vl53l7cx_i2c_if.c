#include "ti_msp_dl_config.h"
#include "vl53l7cx_i2c_if.h"
#include "vl53l7cx_uart_if.h"

#include "dl_gpio.h"
#include "dl_i2c.h"

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

/* Use your SysConfig instance name */
#define I2C_INST   CONFIG_I2C_0_INST

/* Tune these if needed */
#define I2C_TIMEOUT_IDLE      (200000u)
#define I2C_TIMEOUT_BUSFREE   (400000u)
#define I2C_TIMEOUT_RXBYTE    (400000u)
#define I2C_TIMEOUT_TXSPACE   (400000u)

/* ---- address normalization ---- */
static inline uint8_t vl_addr_to_7bit(uint16_t addr)
{
    if (addr <= 0x7Fu) {
        return (uint8_t)addr;
    }
    return (uint8_t)(addr >> 1);
}

/* --------- wait helpers --------- */
static inline int i2c_error(void)
{
    return (DL_I2C_getControllerStatus(I2C_INST) &
            DL_I2C_CONTROLLER_STATUS_ERROR) ? 1 : 0;
}

static int wait_idle(uint32_t timeout)
{
    while (timeout--) {
        uint32_t st = DL_I2C_getControllerStatus(I2C_INST);
        if (st & DL_I2C_CONTROLLER_STATUS_ERROR) return -2;
        if (st & DL_I2C_CONTROLLER_STATUS_IDLE)  return 0;
    }
    return -1;
}

static int wait_bus_free(uint32_t timeout)
{
    while (timeout--) {
        uint32_t st = DL_I2C_getControllerStatus(I2C_INST);
        if (st & DL_I2C_CONTROLLER_STATUS_ERROR) return -2;
        if (!(st & DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) return 0;
    }
    return -1;
}

static int wait_txfifo_space(uint32_t timeout)
{
    while (timeout--) {
        if (i2c_error()) return -2;
        if (!DL_I2C_isControllerTXFIFOFull(I2C_INST)) return 0;
    }
    return -1;
}

static int wait_rxfifo_data(uint32_t timeout)
{
    while (timeout--) {
        if (i2c_error()) return -2;
        if (!DL_I2C_isControllerRXFIFOEmpty(I2C_INST)) return 0;
    }
    return -1;
}

/* --------- public helpers --------- */
void VL_I2C_Init(void)
{
    DL_I2C_enableController(I2C_INST);
}

void VL_WaitMs(uint32_t ms)
{
    while (ms--) delay_cycles(32000);
}

/*void VL_PowerEnable(bool en)
{
    if (en)  DL_GPIO_setPins  (CONFIG_GPIO_PWREN_PORT, CONFIG_GPIO_PWREN_PIN_7_PIN);
    else     DL_GPIO_clearPins(CONFIG_GPIO_PWREN_PORT, CONFIG_GPIO_PWREN_PIN_7_PIN);
}*/

void VL_LowPower(bool en)
{
    if (en)  DL_GPIO_clearPins(CONFIG_GPIO_LPn_PORT, CONFIG_GPIO_LPn_PIN_A5_PIN);
    else     DL_GPIO_setPins  (CONFIG_GPIO_LPn_PORT, CONFIG_GPIO_LPn_PIN_A5_PIN);
}

void VL_I2C_ResetPulse(void)
{
    DL_GPIO_clearPins(CONFIG_GPIO_I2C_RST_PORT, CONFIG_GPIO_I2C_RST_PIN_A24_PIN);
    VL_WaitMs(2);
    DL_GPIO_setPins  (CONFIG_GPIO_I2C_RST_PORT, CONFIG_GPIO_I2C_RST_PIN_A24_PIN);
    VL_WaitMs(2);
    DL_GPIO_clearPins(CONFIG_GPIO_I2C_RST_PORT, CONFIG_GPIO_I2C_RST_PIN_A24_PIN);
    VL_WaitMs(2);
}

/* --------- VL53L7CX low-level I2C --------- */

int32_t VL_I2C_WriteMulti(uint16_t addr, uint16_t reg,
                          const uint8_t *data, uint32_t len)
{
    if (len == 0) return 0;

    uint8_t a7 = vl_addr_to_7bit(addr);
    const uint32_t CHUNK = 256;

    uint32_t remaining = len;
    uint16_t cur_reg   = reg;
    const uint8_t *p   = data;

    while (remaining > 0)
    {
        uint32_t n = (remaining > CHUNK) ? CHUNK : remaining;

        if (wait_idle(I2C_TIMEOUT_IDLE) < 0) return -10;
        if (wait_bus_free(I2C_TIMEOUT_BUSFREE) < 0) return -11;

        DL_I2C_resetControllerTransfer(I2C_INST);
        DL_I2C_disableControllerReadOnTXEmpty(I2C_INST);

        DL_I2C_startControllerTransfer(I2C_INST, a7,
                                       DL_I2C_CONTROLLER_DIRECTION_TX,
                                       (uint16_t)(n + 2u));

        if (wait_txfifo_space(I2C_TIMEOUT_TXSPACE) < 0) return -12;
        DL_I2C_transmitControllerData(I2C_INST, (uint8_t)(cur_reg >> 8));

        if (wait_txfifo_space(I2C_TIMEOUT_TXSPACE) < 0) return -13;
        DL_I2C_transmitControllerData(I2C_INST, (uint8_t)(cur_reg & 0xFF));

        for (uint32_t i = 0; i < n; i++)
        {
            if (wait_txfifo_space(I2C_TIMEOUT_TXSPACE) < 0) return -14;
            DL_I2C_transmitControllerData(I2C_INST, p[i]);
        }

        if (wait_bus_free(I2C_TIMEOUT_BUSFREE) < 0) return -15;
        if (i2c_error()) return -16;

        remaining -= n;
        p += n;
        cur_reg = (uint16_t)(cur_reg + (uint16_t)n);
    }

    return 0;
}

int32_t VL_I2C_ReadMulti(uint16_t addr, uint16_t reg,
                         uint8_t *data, uint32_t len)
{
    if (len == 0) return 0;

    uint8_t a7 = vl_addr_to_7bit(addr);
    uint8_t h0 = (uint8_t)(reg >> 8);
    uint8_t h1 = (uint8_t)(reg & 0xFF);

    if (wait_idle(I2C_TIMEOUT_IDLE) < 0) return -20;
    if (wait_bus_free(I2C_TIMEOUT_BUSFREE) < 0) return -21;

    DL_I2C_resetControllerTransfer(I2C_INST);
    DL_I2C_enableControllerReadOnTXEmpty(I2C_INST);

    if (wait_txfifo_space(I2C_TIMEOUT_TXSPACE) < 0) return -22;
    DL_I2C_transmitControllerData(I2C_INST, h0);

    if (wait_txfifo_space(I2C_TIMEOUT_TXSPACE) < 0) return -23;
    DL_I2C_transmitControllerData(I2C_INST, h1);

    if (wait_idle(I2C_TIMEOUT_IDLE) < 0) return -24;

    DL_I2C_startControllerTransfer(I2C_INST, a7,
                                   DL_I2C_CONTROLLER_DIRECTION_RX,
                                   (uint16_t)len);

    for (uint32_t i = 0; i < len; i++) {
        int w = wait_rxfifo_data(I2C_TIMEOUT_RXBYTE);
        if (w < 0) return -25;
        data[i] = DL_I2C_receiveControllerData(I2C_INST);
    }

    DL_I2C_resetControllerTransfer(I2C_INST);
    DL_I2C_disableControllerReadOnTXEmpty(I2C_INST);

    if (wait_bus_free(I2C_TIMEOUT_BUSFREE) < 0) return -26;
    if (i2c_error()) return -27;

    return 0;
}
