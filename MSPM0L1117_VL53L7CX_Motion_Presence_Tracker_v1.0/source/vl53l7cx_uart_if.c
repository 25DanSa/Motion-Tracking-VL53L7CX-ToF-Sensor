#include "ti_msp_dl_config.h"
#include "vl53l7cx_uart_if.h"

#include "dl_uart.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* -------------------------------------------------
   SINGLE UART IMPLEMENTATION
   UART0 ONLY
   TX: PA10 -> ESP32
   RX: PA11 <- ESP32
--------------------------------------------------*/

/* ---------- TX ---------- */
static void uart0_tx_blocking(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        DL_UART_transmitDataBlocking(CONFIG_UART_0_INST, data[i]);
    }
}

void UartJson_Write(const uint8_t *data, uint32_t len)
{
    if (!data || len == 0) return;
    uart0_tx_blocking(data, len);
}

void UartJson_Println(const char *s)
{
    if (!s) return;

    uart0_tx_blocking((const uint8_t*)s, strlen(s));
    uart0_tx_blocking((const uint8_t*)"\r\n", 2);
}

/* ---- compatibility for platform.c ---- */
void Uart0_Print(const char *s)
{
    UartJson_Println(s);
}

/* ---------- RX ---------- */

bool UartJson_Available(void)
{
    return !DL_UART_Main_isRXFIFOEmpty(CONFIG_UART_0_INST);
}

bool UartJson_ReadByte(uint8_t *out)
{
    if (!out) return false;
    if (!UartJson_Available()) return false;

    *out = (uint8_t)DL_UART_Main_receiveData(CONFIG_UART_0_INST);
    return true;
}

bool UartJson_ReadLine(char *buf, uint32_t bufSize)
{
    static uint32_t idx = 0;
    uint8_t c;

    if (!buf || bufSize < 2) return false;

    while (UartJson_ReadByte(&c)) {

        if (c == '\r') {
            continue;
        }

        if (c == '\n') {
            buf[idx] = '\0';
            idx = 0;
            return true;
        }

        if (idx < (bufSize - 1U)) {
            buf[idx++] = (char)c;
        } else {
            /* overflow: reset line */
            idx = 0;
            buf[0] = '\0';
        }
    }

    return false;
}