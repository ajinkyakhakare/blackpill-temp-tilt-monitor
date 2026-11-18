#include "lm35.h"
#include "config.h"

/* The Black Pill ties VDDA to the 3.3 V rail, so that is the ADC reference.
 * Any error in the real 3.3 V shows up directly as a temperature error. */
#define ADC_VREF_MV      3300.0f
#define ADC_MAX_COUNT    4095.0f   /* 12-bit ADC */
#define LM35_MV_PER_C    10.0f     /* from the LM35 datasheet */

/* One conversion takes well under 1 ms, so 2 ms only trips if the ADC is
 * misconfigured. */
#define ADC_TIMEOUT_MS   2U

HAL_StatusTypeDef LM35_ReadCelsius(ADC_HandleTypeDef *hadc, float *temp_c)
{
    uint32_t sum = 0;

    for (uint32_t i = 0; i < LM35_SAMPLES; i++) {
        /* Polling one conversion at a time is simpler than DMA, and 16
         * conversions every 100 ms is a tiny amount of CPU time. */
        HAL_ADC_Start(hadc);
        if (HAL_ADC_PollForConversion(hadc, ADC_TIMEOUT_MS) != HAL_OK) {
            HAL_ADC_Stop(hadc);
            return HAL_ERROR;
        }
        sum += HAL_ADC_GetValue(hadc);
    }
    HAL_ADC_Stop(hadc);

    /* Averaging before converting keeps the maths to one division chain
     * and smooths out ADC noise of a few counts. */
    float avg_counts = (float)sum / (float)LM35_SAMPLES;
    float millivolts = avg_counts * ADC_VREF_MV / ADC_MAX_COUNT;

    *temp_c = millivolts / LM35_MV_PER_C;
    return HAL_OK;
}
