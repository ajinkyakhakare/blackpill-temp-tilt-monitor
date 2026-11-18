#ifndef MPU6050_H
#define MPU6050_H

#include "stm32f4xx_hal.h"

/* 7-bit address with AD0 low (the GY-521 board pulls AD0 low). */
#define MPU6050_ADDR_7BIT      0x68U

/* Value a genuine MPU6050 returns from WHO_AM_I. Some clone boards carry an
 * MPU6500 or similar and return something else (0x70 is common). */
#define MPU6050_WHO_AM_I_VAL   0x68U

typedef struct {
    float ax_g;
    float ay_g;
    float az_g;
} MPU6050_Accel;

/* Checks WHO_AM_I, wakes the chip and sets the accel range and filter.
 * Returns HAL_ERROR if the chip does not answer or is not an MPU6050. */
HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c);

/* Reads all three accel axes in one 6-byte burst and converts to g. */
HAL_StatusTypeDef MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, MPU6050_Accel *out);

/* Angle between the sensor Z axis and gravity, 0 to 180 degrees. */
float MPU6050_TiltDeg(const MPU6050_Accel *a);

/* Total acceleration magnitude in g (about 1.0 at rest). */
float MPU6050_MagnitudeG(const MPU6050_Accel *a);

#endif /* MPU6050_H */
