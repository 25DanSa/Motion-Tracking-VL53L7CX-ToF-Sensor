#include "platform.h"

#include "vl53l7cx_api.h"
#include "vl53l7cx_i2c_if.h"
#include "vl53l7cx_uart_if.h"

#include <stdio.h>
#include <string.h>

static inline uint16_t get_addr(VL53L7CX_Platform *p)
{
    if (p && p->address) return p->address;  /* you pass 0x52 */
    return VL53L7CX_DEFAULT_I2C_ADDRESS;     /* 0x52 */
}

static void print_i2c_err(const char *tag, int32_t rc, uint16_t reg, uint32_t size)
{
    char b[96];
    snprintf(b, sizeof(b), "%s rc=%ld reg=%04X size=%lu\r\n",
             tag, (long)rc, (unsigned)reg, (unsigned long)size);
    Uart0_Print(b);
}

uint8_t VL53L7CX_RdByte(VL53L7CX_Platform *p, uint16_t reg, uint8_t *v)
{
    int32_t rc = VL_I2C_ReadMulti(get_addr(p), reg, v, 1);
    if (rc != 0) { print_i2c_err("RdByte", rc, reg, 1); return 255; }
    return 0;
}

uint8_t VL53L7CX_WrByte(VL53L7CX_Platform *p, uint16_t reg, uint8_t v)
{
    int32_t rc = VL_I2C_WriteMulti(get_addr(p), reg, &v, 1);
    if (rc != 0) { print_i2c_err("WrByte", rc, reg, 1); return 255; }
    return 0;
}

uint8_t VL53L7CX_WrMulti(VL53L7CX_Platform *p, uint16_t reg, uint8_t *buf, uint32_t size)
{
    int32_t rc = VL_I2C_WriteMulti(get_addr(p), reg, buf, size);
    if (rc != 0) { print_i2c_err("WrMulti", rc, reg, size); return 255; }
    return 0;
}

uint8_t VL53L7CX_RdMulti(VL53L7CX_Platform *p, uint16_t reg, uint8_t *buf, uint32_t size)
{
    int32_t rc = VL_I2C_ReadMulti(get_addr(p), reg, buf, size);
    if (rc != 0) { print_i2c_err("RdMulti", rc, reg, size); return 255; }
    return 0;
}

uint8_t VL53L7CX_Reset_Sensor(VL53L7CX_Platform *p_platform)
{
    (void)p_platform;
    /* keep empty (you already do power sequence in main) */
    return 0;
}

void VL53L7CX_SwapBuffer(uint8_t *buffer, uint16_t size)
{
    uint32_t i, tmp;
    for (i = 0; i < size; i += 4) {
        tmp = ((uint32_t)buffer[i] << 24) |
              ((uint32_t)buffer[i+1] << 16) |
              ((uint32_t)buffer[i+2] << 8) |
              ((uint32_t)buffer[i+3]);
        memcpy(&buffer[i], &tmp, 4);
    }
}

uint8_t VL53L7CX_WaitMs(VL53L7CX_Platform *p_platform, uint32_t ms)
{
    (void)p_platform;
    VL_WaitMs(ms);
    return 0;
}
