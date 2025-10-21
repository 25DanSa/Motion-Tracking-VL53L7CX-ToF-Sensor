// vl53l7cx_uart_if.c
#include "ti_msp_dl_config.h"
#include "vl53l7cx_uart_if.h"
#include <stdio.h>
#include <stdarg.h>

#include "dl_uart.h"
#include <string.h>


void Uart0_Print(const char *s)
{
    const uint8_t *p = (const uint8_t*)s;
    for (size_t i = 0; i < strlen(s); ++i) {
        DL_UART_Main_transmitDataBlocking(CONFIG_UART_0_INST, p[i]);
    }
}

void Uart1_Write(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; ++i) {
        DL_UART_Main_transmitDataBlocking(CONFIG_UART_1_INST, data[i]);
    }
}

void Uart0_Printf(const char *fmt, ...)
{
    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    Uart0_Print(buf);
}