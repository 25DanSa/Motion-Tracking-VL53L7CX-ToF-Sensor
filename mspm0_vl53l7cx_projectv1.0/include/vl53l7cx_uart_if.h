// vl53l7cx_uart_if.h
#ifndef VL53L7CX_UART_IF_H
#define VL53L7CX_UART_IF_H
#include <stdint.h>
void Uart0_Print(const char *s);
void Uart0_Printf(const char *fmt, ...);
void Uart1_Write(const uint8_t *data, uint32_t len);
#endif
