#ifndef LM35_H
#define LM35_H

#include "stm32f4xx_hal.h"

/* Reads LM35_SAMPLES conversions from the given ADC, averages them and
 * converts to degrees Celsius. Returns HAL_ERROR if a conversion times out,
 * in which case *temp_c is left unchanged. */
HAL_StatusTypeDef LM35_ReadCelsius(ADC_HandleTypeDef *hadc, float *temp_c);

#endif /* LM35_H */
