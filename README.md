/*
 * MCXN947 (FRDM-MCXN947) demo:
 *  - Read potentiometer via LPADC (12-bit)
 *  - Print ADC + duty over serial (PRINTF via Debug Console)
 *  - Use ADC to set FlexPWM duty (motor speed control via external transistor/MOSFET driver)
 *
 * What you MUST configure in Pins/Clocks (MCUXpresso Config Tools):
 *  1) The ADC input pin -> set to analog, routed to the LPADC channel you choose.
 *  2) A FlexPWM output pin (e.g. PWM1_A0 / PWM1_A1 / etc.) -> pin mux to PWM function.
 *  3) Debug console UART (or USB-CDC if your project uses that) for PRINTF().
 *
 * Build notes:
 *  - Uses: fsl_lpadc.h and fsl_pwm.h and fsl_debug_console.h
 *  - Assumes 12-bit ADC (0..4095). If you configure different resolution, update ADC_MAX_COUNTS.
 */

#include <stdint.h>
#include <stdbool.h>

#include "fsl_debug_console.h"
#include "board.h"
#include "clock_config.h"
#include "pin_mux.h"

#include "fsl_lpadc.h"
#include "fsl_pwm.h"

/* ===================== USER SETTINGS ===================== */

/* ---- ADC ----
 * Set these to match YOUR analog pin/channel in the Pins tool.
 * On MCX N, the LPADC driver typically uses ADC0/ADC1 as bases.
 */
#define ADC_BASE            ADC0
#define ADC_CHANNEL_NUMBER  0U            /* <-- CHANGE: LPADC channel number for your pot pin */
#define ADC_CMD_ID          1U
#define ADC_TRIG_ID         0U
#define ADC_MAX_COUNTS      4095U         /* 12-bit */

/* ---- PWM (FlexPWM) ----
 * Choose PWM instance + submodule + A/B output that matches your pin mux.
 * Example: PWM1 submodule 0, output A.
 */
#define PWM_BASE            PWM1
#define PWM_SUBMODULE       kPWM_Module_0 /* <-- CHANGE if needed */
#define PWM_OUTPUT_CHANNEL  kPWM_PwmA     /* kPWM_PwmA or kPWM_PwmB */
#define PWM_ALIGN_MODE      kPWM_EdgeAligned

#define PWM_FREQ_HZ         10000U        /* 10 kHz is a good motor PWM starting point */
#define PWM_SRC_CLK_HZ      CLOCK_GetFreq(kCLOCK_BusClk)

/* For easy scaling, we choose a fixed "period counts" value used ONLY for math.
 * FlexPWM period is actually set by PWM_SetupPwm() based on PWM_FREQ_HZ + PWM_SRC_CLK_HZ,
 * so we update duty by percent (0..100).
 */
#define SERIAL_PRINT_EVERY_N  10U         /* reduce spam: print once every N samples */

/* ===================== FORWARD DECLARATIONS ===================== */
static void     APP_InitAdc(void);
static uint16_t APP_ReadAdcBlocking(void);

static void     APP_InitPwm(void);
static void     APP_SetPwmDutyPercent(uint8_t dutyPercent);

static uint8_t  APP_MapAdcToDutyPercent(uint16_t adcRaw);

/* ===================== MAIN ===================== */

int main(void)
{
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    PRINTF("\r\nMCXN947 ADC->Serial->PWM demo starting...\r\n");

    APP_InitAdc();
    APP_InitPwm();

    uint32_t sampleCount = 0;

    while (1)
    {
        uint16_t adc = APP_ReadAdcBlocking();
        uint8_t duty = APP_MapAdcToDutyPercent(adc);

        APP_SetPwmDutyPercent(duty);

        /* Serial output (view in a terminal at your debug console settings, commonly 115200 8N1) */
        if ((sampleCount++ % SERIAL_PRINT_EVERY_N) == 0U)
        {
            PRINTF("ADC=%u, Duty=%u%%\r\n", adc, duty);
        }

        /* Simple delay to keep loop reasonable (replace with SDK_DelayAtLeastUs if you prefer) */
        for (volatile uint32_t i = 0; i < 100000U; i++) { __NOP(); }
    }
}

/* ===================== ADC (LPADC) ===================== */

static void APP_InitAdc(void)
{
    lpadc_config_t adcConfig;
    LPADC_GetDefaultConfig(&adcConfig);

    /* If you want, you can tweak:
     * adcConfig.enableAnalogPreliminary = true/false (depends on SDK)
     * adcConfig.powerLevelMode = ...
     */

    LPADC_Init(ADC_BASE, &adcConfig);

    /* Some SDKs provide auto-calibration. If your SDK has it, uncomment:
     * LPADC_DoAutoCalibration(ADC_BASE);
     */

    /* Command: chooses channel + sampling mode, averaging, etc. */
    lpadc_conv_command_config_t cmdConfig;
    LPADC_GetDefaultConvCommandConfig(&cmdConfig);
    cmdConfig.channelNumber = ADC_CHANNEL_NUMBER;
    cmdConfig.sampleChannelMode = kLPADC_SampleChannelSingleEndSideA; /* typical single-ended */
    LPADC_SetConvCommandConfig(ADC_BASE, ADC_CMD_ID, &cmdConfig);

    /* Trigger: software trigger -> our command */
    lpadc_conv_trigger_config_t trigConfig;
    LPADC_GetDefaultConvTriggerConfig(&trigConfig);
    trigConfig.targetCommandId       = ADC_CMD_ID;
    trigConfig.enableHardwareTrigger = false;
    LPADC_SetConvTriggerConfig(ADC_BASE, ADC_TRIG_ID, &trigConfig);

    PRINTF("ADC init done (channel %u).\r\n", (unsigned)ADC_CHANNEL_NUMBER);
}

static uint16_t APP_ReadAdcBlocking(void)
{
    lpadc_conv_result_t result;

    /* Start conversion using software trigger ID */
    LPADC_DoSoftwareTrigger(ADC_BASE, (1U << ADC_TRIG_ID));

    /* Wait for result */
    while (!LPADC_GetConvResult(ADC_BASE, &result))
    {
        /* spin */
    }

    return (uint16_t)result.convValue;
}

/* ===================== PWM (FlexPWM via fsl_pwm.h) ===================== */

static void APP_InitPwm(void)
{
    pwm_config_t pwmConfig;
    PWM_GetDefaultConfig(&pwmConfig);

    /* Init PWM module */
    PWM_Init(PWM_BASE, &pwmConfig);

    /* Configure one output signal (A or B on chosen submodule) */
    pwm_signal_param_t pwmSignal;
    pwmSignal.pwmChannel       = PWM_OUTPUT_CHANNEL;
    pwmSignal.level            = kPWM_HighTrue;
    pwmSignal.dutyCyclePercent = 0U;              /* start stopped */
    pwmSignal.deadtimeValue    = 0U;
    pwmSignal.faultState       = kPWM_PwmFaultState0;
    pwmSignal.pwmchannelenable = true;

    PWM_SetupPwm(PWM_BASE,
                 PWM_SUBMODULE,
                 &pwmSignal,
                 1U,
                 PWM_ALIGN_MODE,
                 PWM_FREQ_HZ,
                 PWM_SRC_CLK_HZ);

    /* Load registers then start */
    PWM_SetPwmLdok(PWM_BASE, (1U << (uint8_t)PWM_SUBMODULE), true);
    PWM_StartTimer(PWM_BASE, (1U << (uint8_t)PWM_SUBMODULE));

    PRINTF("PWM init done (%u Hz).\r\n", (unsigned)PWM_FREQ_HZ);
}

static void APP_SetPwmDutyPercent(uint8_t dutyPercent)
{
    if (dutyPercent > 100U) dutyPercent = 100U;

    PWM_UpdatePwmDutycycle(PWM_BASE,
                           PWM_SUBMODULE,
                           PWM_OUTPUT_CHANNEL,
                           PWM_ALIGN_MODE,
                           dutyPercent);

    /* Ensure the updated duty loads */
    PWM_SetPwmLdok(PWM_BASE, (1U << (uint8_t)PWM_SUBMODULE), true);
}

/* ===================== Mapping ===================== */

static uint8_t APP_MapAdcToDutyPercent(uint16_t adcRaw)
{
    /* Clamp in case of unexpected values */
    if (adcRaw > ADC_MAX_COUNTS) adcRaw = ADC_MAX_COUNTS;

    /* duty% = adcRaw / ADC_MAX * 100 (with rounding) */
    uint32_t duty = ((uint32_t)adcRaw * 100U + (ADC_MAX_COUNTS / 2U)) / ADC_MAX_COUNTS;
    if (duty > 100U) duty = 100U;

    return (uint8_t)duty;
}
