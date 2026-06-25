/*
 * Copyright (c) 2023, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.c =============
 *  Configured MSPM0 DriverLib module definitions
 *
 *  DO NOT EDIT - This file is generated for the MSPM0L111X
 *  by the SysConfig tool.
 */

#include "ti_msp_dl_config.h"

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform any initialization needed before using any board APIs
 */
SYSCONFIG_WEAK void SYSCFG_DL_init(void)
{
    SYSCFG_DL_initPower();
    SYSCFG_DL_GPIO_init();
    /* Module-Specific Initializations*/
    SYSCFG_DL_SYSCTL_init();
    SYSCFG_DL_CONFIG_I2C_0_init();
    SYSCFG_DL_CONFIG_UART_0_init();
    SYSCFG_DL_CONFIG_UART_1_init();
}

SYSCONFIG_WEAK void SYSCFG_DL_initPower(void)
{
    DL_GPIO_reset(GPIOA);
    DL_I2C_reset(CONFIG_I2C_0_INST);
    DL_UART_Main_reset(CONFIG_UART_0_INST);
    DL_UART_Main_reset(CONFIG_UART_1_INST);

    DL_GPIO_enablePower(GPIOA);
    DL_I2C_enablePower(CONFIG_I2C_0_INST);
    DL_UART_Main_enablePower(CONFIG_UART_0_INST);
    DL_UART_Main_enablePower(CONFIG_UART_1_INST);
    delay_cycles(POWER_STARTUP_DELAY);
}

SYSCONFIG_WEAK void SYSCFG_DL_GPIO_init(void)
{

    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_CONFIG_I2C_0_IOMUX_SDA,
        GPIO_CONFIG_I2C_0_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_CONFIG_I2C_0_IOMUX_SCL,
        GPIO_CONFIG_I2C_0_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_CONFIG_I2C_0_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_CONFIG_I2C_0_IOMUX_SCL);

    DL_GPIO_initPeripheralOutputFunction(
        GPIO_CONFIG_UART_0_IOMUX_TX, GPIO_CONFIG_UART_0_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_CONFIG_UART_0_IOMUX_RX, GPIO_CONFIG_UART_0_IOMUX_RX_FUNC);
    DL_GPIO_initPeripheralOutputFunction(
        GPIO_CONFIG_UART_1_IOMUX_TX, GPIO_CONFIG_UART_1_IOMUX_TX_FUNC);
    DL_GPIO_initPeripheralInputFunction(
        GPIO_CONFIG_UART_1_IOMUX_RX, GPIO_CONFIG_UART_1_IOMUX_RX_FUNC);

    DL_GPIO_initDigitalOutput(CONFIG_GPIO_LPn_PIN_A5_IOMUX);

    DL_GPIO_initDigitalOutput(CONFIG_GPIO_I2C_RST_PIN_A24_IOMUX);

    DL_GPIO_initDigitalInputFeatures(CONFIG_GPIO_INT_PIN_A26_IOMUX,
		 DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
		 DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);

    DL_GPIO_clearPins(GPIOA, CONFIG_GPIO_LPn_PIN_A5_PIN);
    DL_GPIO_setPins(GPIOA, CONFIG_GPIO_I2C_RST_PIN_A24_PIN);
    DL_GPIO_enableOutput(GPIOA, CONFIG_GPIO_LPn_PIN_A5_PIN |
		CONFIG_GPIO_I2C_RST_PIN_A24_PIN);

}


SYSCONFIG_WEAK void SYSCFG_DL_SYSCTL_init(void)
{

	//Low Power Mode is configured to be SLEEP0
    DL_SYSCTL_setBORThreshold(DL_SYSCTL_BOR_THRESHOLD_LEVEL_0);

    DL_SYSCTL_setSYSOSCFreq(DL_SYSCTL_SYSOSC_FREQ_BASE);
    DL_SYSCTL_setMCLKDivider(DL_SYSCTL_MCLK_DIVIDER_DISABLE);

}


static const DL_I2C_ClockConfig gCONFIG_I2C_0ClockConfig = {
    .clockSel = DL_I2C_CLOCK_BUSCLK,
    .divideRatio = DL_I2C_CLOCK_DIVIDE_1,
};

SYSCONFIG_WEAK void SYSCFG_DL_CONFIG_I2C_0_init(void) {

    DL_I2C_setClockConfig(CONFIG_I2C_0_INST,
        (DL_I2C_ClockConfig *) &gCONFIG_I2C_0ClockConfig);
    DL_I2C_setAnalogGlitchFilterPulseWidth(CONFIG_I2C_0_INST,
        DL_I2C_ANALOG_GLITCH_FILTER_WIDTH_50NS);
    DL_I2C_enableAnalogGlitchFilter(CONFIG_I2C_0_INST);
    DL_I2C_setDigitalGlitchFilterPulseWidth(CONFIG_I2C_0_INST,
        DL_I2C_DIGITAL_GLITCH_FILTER_WIDTH_CLOCKS_1);

    /* Configure Controller Mode */
    DL_I2C_resetControllerTransfer(CONFIG_I2C_0_INST);
    /* Set frequency to 100000 Hz*/
    DL_I2C_setTimerPeriod(CONFIG_I2C_0_INST, 31);
    DL_I2C_setControllerTXFIFOThreshold(CONFIG_I2C_0_INST, DL_I2C_TX_FIFO_LEVEL_BYTES_7);
    DL_I2C_setControllerRXFIFOThreshold(CONFIG_I2C_0_INST, DL_I2C_RX_FIFO_LEVEL_BYTES_8);
    DL_I2C_enableControllerClockStretching(CONFIG_I2C_0_INST);


    /* Enable module */
    DL_I2C_enableController(CONFIG_I2C_0_INST);


}

static const DL_UART_Main_ClockConfig gCONFIG_UART_0ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gCONFIG_UART_0Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_CONFIG_UART_0_init(void)
{
    DL_UART_Main_setClockConfig(CONFIG_UART_0_INST, (DL_UART_Main_ClockConfig *) &gCONFIG_UART_0ClockConfig);

    DL_UART_Main_init(CONFIG_UART_0_INST, (DL_UART_Main_Config *) &gCONFIG_UART_0Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 921600
     *  Actual baud rate: 920863.31
     */
    DL_UART_Main_setOversampling(CONFIG_UART_0_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(CONFIG_UART_0_INST, CONFIG_UART_0_IBRD_32_MHZ_921600_BAUD, CONFIG_UART_0_FBRD_32_MHZ_921600_BAUD);



    DL_UART_Main_enable(CONFIG_UART_0_INST);
}
static const DL_UART_Main_ClockConfig gCONFIG_UART_1ClockConfig = {
    .clockSel    = DL_UART_MAIN_CLOCK_BUSCLK,
    .divideRatio = DL_UART_MAIN_CLOCK_DIVIDE_RATIO_1
};

static const DL_UART_Main_Config gCONFIG_UART_1Config = {
    .mode        = DL_UART_MAIN_MODE_NORMAL,
    .direction   = DL_UART_MAIN_DIRECTION_TX_RX,
    .flowControl = DL_UART_MAIN_FLOW_CONTROL_NONE,
    .parity      = DL_UART_MAIN_PARITY_NONE,
    .wordLength  = DL_UART_MAIN_WORD_LENGTH_8_BITS,
    .stopBits    = DL_UART_MAIN_STOP_BITS_ONE
};

SYSCONFIG_WEAK void SYSCFG_DL_CONFIG_UART_1_init(void)
{
    DL_UART_Main_setClockConfig(CONFIG_UART_1_INST, (DL_UART_Main_ClockConfig *) &gCONFIG_UART_1ClockConfig);

    DL_UART_Main_init(CONFIG_UART_1_INST, (DL_UART_Main_Config *) &gCONFIG_UART_1Config);
    /*
     * Configure baud rate by setting oversampling and baud rate divisors.
     *  Target baud rate: 115200
     *  Actual baud rate: 115211.52
     */
    DL_UART_Main_setOversampling(CONFIG_UART_1_INST, DL_UART_OVERSAMPLING_RATE_16X);
    DL_UART_Main_setBaudRateDivisor(CONFIG_UART_1_INST, CONFIG_UART_1_IBRD_32_MHZ_115200_BAUD, CONFIG_UART_1_FBRD_32_MHZ_115200_BAUD);



    DL_UART_Main_enable(CONFIG_UART_1_INST);
}

