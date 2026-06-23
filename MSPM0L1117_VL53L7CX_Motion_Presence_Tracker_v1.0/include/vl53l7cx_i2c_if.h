// vl53l7cx_i2c_if.h
#ifndef VL53L7CX_I2C_IF_H
#define VL53L7CX_I2C_IF_H
#include <stdint.h>
#include <stdbool.h>
void     VL_I2C_Init(void);
int32_t  VL_I2C_WriteMulti(uint16_t addr, uint16_t reg, const uint8_t *data, uint32_t len);
int32_t  VL_I2C_ReadMulti(uint16_t addr, uint16_t reg, uint8_t *data, uint32_t len);
void     VL_WaitMs(uint32_t ms);
void     VL_PowerEnable(bool en);
void     VL_LowPower(bool en);
void     VL_I2C_ResetPulse(void);
#endif
