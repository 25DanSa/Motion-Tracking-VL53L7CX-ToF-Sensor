#include "ti_msp_dl_config.h"
#include "vl53l7cx_i2c_if.h"

#include "dl_gpio.h"
#include "dl_i2c.h"
#include <string.h>
#include <stdbool.h>

/* Helper: wait until controller is idle */
static inline void I2C_WaitIdle(void)
{
    /* One of these exists in 2.07.00.05; pick the one your header exposes. */
#if defined(DL_I2C_isControllerBusy)
    while (DL_I2C_isControllerBusy(CONFIG_I2C_0_INST)) { /* wait */ }
#elif defined(DL_I2C_isBusBusy)
    while (DL_I2C_isBusBusy(CONFIG_I2C_0_INST)) { /* wait */ }
#else
    /* Fallback: short spin — adjust if needed */
    for (volatile uint32_t t=0; t<20000; ++t) { __NOP(); }
#endif
}

void VL_I2C_Init(void)
{
    DL_I2C_enableController(CONFIG_I2C_0_INST);
}

int32_t VL_I2C_WriteMulti(uint16_t addr, uint16_t reg,
                          const uint8_t *data, uint32_t len)
{
    uint8_t hdr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };

    /* Push header then payload into TX FIFO */
    for (uint32_t i = 0; i < 2; i++) {
        while (DL_I2C_isControllerTXFIFOFull(CONFIG_I2C_0_INST));
        DL_I2C_transmitControllerData(CONFIG_I2C_0_INST, hdr[i]);
    }
    for (uint32_t i = 0; i < len; i++) {
        while (DL_I2C_isControllerTXFIFOFull(CONFIG_I2C_0_INST));
        DL_I2C_transmitControllerData(CONFIG_I2C_0_INST, data[i]);
    }

    /* Single TX with STOP at end */
    DL_I2C_startControllerTransfer(CONFIG_I2C_0_INST, (uint8_t)addr,
                                   DL_I2C_CONTROLLER_DIRECTION_TX,
                                   DL_I2C_CONTROLLER_STOP_ENABLE);
    I2C_WaitIdle();
    return 0;
}

int32_t VL_I2C_ReadMulti(uint16_t addr, uint16_t reg,
                         uint8_t *data, uint32_t len)
{
    uint8_t hdr[2] = { (uint8_t)(reg >> 8), (uint8_t)(reg & 0xFF) };

    /* TX register address, NO STOP (so we can repeated-start) */
    for (uint32_t i = 0; i < 2; i++) {
        while (DL_I2C_isControllerTXFIFOFull(CONFIG_I2C_0_INST));
        DL_I2C_transmitControllerData(CONFIG_I2C_0_INST, hdr[i]);
    }
    DL_I2C_startControllerTransfer(CONFIG_I2C_0_INST, (uint8_t)addr,
                                   DL_I2C_CONTROLLER_DIRECTION_TX,
                                   DL_I2C_CONTROLLER_STOP_DISABLE);
    I2C_WaitIdle();

    /* RX len bytes, STOP at end */
    DL_I2C_startControllerTransfer(CONFIG_I2C_0_INST, (uint8_t)addr,
                                   DL_I2C_CONTROLLER_DIRECTION_RX,
                                   DL_I2C_CONTROLLER_STOP_ENABLE);

    for (uint32_t i = 0; i < len; i++) {
        while (DL_I2C_isControllerRXFIFOEmpty(CONFIG_I2C_0_INST));
        data[i] = DL_I2C_receiveControllerData(CONFIG_I2C_0_INST);
    }
    I2C_WaitIdle();
    return 0;
}

void VL_WaitMs(uint32_t ms)
{
    while (ms--) delay_cycles(32000); // ≈1 ms @ 32 MHz
}

void VL_PowerEnable(bool en)
{
    if (en)  DL_GPIO_setPins  (CONFIG_GPIO_PWREN_PORT, CONFIG_GPIO_PWREN_PIN_7_PIN);
    else     DL_GPIO_clearPins(CONFIG_GPIO_PWREN_PORT, CONFIG_GPIO_PWREN_PIN_7_PIN);
}

void VL_LowPower(bool en)
{
    if (en)  DL_GPIO_setPins  (CONFIG_GPIO_LPn_PORT, CONFIG_GPIO_LPn_PIN_6_PIN);
    else     DL_GPIO_clearPins(CONFIG_GPIO_LPn_PORT, CONFIG_GPIO_LPn_PIN_6_PIN);
}

void VL_I2C_ResetPulse(void)
{
    DL_GPIO_clearPins(CONFIG_GPIO_I2C_RST_PORT, CONFIG_GPIO_I2C_RST_PIN_5_PIN);
    VL_WaitMs(5);
    DL_GPIO_setPins  (CONFIG_GPIO_I2C_RST_PORT, CONFIG_GPIO_I2C_RST_PIN_5_PIN);
}
