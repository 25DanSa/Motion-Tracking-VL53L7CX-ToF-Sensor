#ifndef VL53L7CX_UART_IF_H
#define VL53L7CX_UART_IF_H

#include <stdint.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Single UART system (UART0 only) */

void UartJson_Write(const uint8_t *data, uint32_t len);
void UartJson_Println(const char *s);

/* compatibility (used by platform.c) */
void Uart0_Print(const char *s);

/* RX helpers */
bool UartJson_Available(void);
bool UartJson_ReadByte(uint8_t *out);
bool UartJson_ReadLine(char *buf, uint32_t bufSize);

#ifdef __cplusplus
}
#endif

#endif