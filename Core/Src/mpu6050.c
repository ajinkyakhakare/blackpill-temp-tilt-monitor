#include "mpu6050.h"
#include <math.h>

/* HAL expects the address already shifted into the upper 7 bits, with bit 0
 * left free for the read/write bit. Passing plain 0x68 gives no ACK. */
#define MPU6050_I2C_ADDR     (MPU6050_ADDR_7BIT << 1)

#define REG_CONFIG           0x1AU
#define REG_ACCEL_CONFIG     0x1CU
#define REG_ACCEL_XOUT_H     0x3BU
#define REG_PWR_MGMT_1       0x6BU
#define REG_WHO_AM_I         0x75U

/* PWR_MGMT_1: clear SLEEP (the chip powers up asleep) and use the X gyro PLL
 * as clock, which the datasheet recommends over the internal oscillator. */
#define PWR_MGMT_1_WAKE      0x01U

/* CONFIG: digital low-pass filter at about 44 Hz. Enough to cut vibration
 * noise while still passing a sharp knock at a 50 Hz read rate. */
#define CONFIG_DLPF_44HZ     0x03U

/* ACCEL_CONFIG: +/-4 g full scale, 8192 counts per g. The default +/-2 g
 * would clip right at the 2 g shock limit. */
#define ACCEL_CONFIG_4G      0x08U
#define ACCEL_COUNTS_PER_G   8192.0f

#define I2C_TIMEOUT_MS       10U

#define RAD_TO_DEG           (180.0f / 3.14159265f)

static HAL_StatusTypeDef write_reg(I2C_HandleTypeDef *hi2c, uint8_t reg, uint8_t val)
{
    return HAL_I2C_Mem_Write(hi2c, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                             &val, 1, I2C_TIMEOUT_MS);
}

static HAL_StatusTypeDef read_regs(I2C_HandleTypeDef *hi2c, uint8_t reg,
                                   uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Mem_Read(hi2c, MPU6050_I2C_ADDR, reg, I2C_MEMADD_SIZE_8BIT,
                            buf, len, I2C_TIMEOUT_MS);
}

HAL_StatusTypeDef MPU6050_Init(I2C_HandleTypeDef *hi2c)
{
    uint8_t id = 0;

    /* Checking the ID first means a wrong chip or bad wiring fails here,
     * instead of producing plausible-looking garbage later. */
    if (read_regs(hi2c, REG_WHO_AM_I, &id, 1) != HAL_OK) {
        return HAL_ERROR;
    }
    if (id != MPU6050_WHO_AM_I_VAL) {
        return HAL_ERROR;
    }

    if (write_reg(hi2c, REG_PWR_MGMT_1, PWR_MGMT_1_WAKE) != HAL_OK) {
        return HAL_ERROR;
    }
    /* Give the clock source time to settle. This runs once at startup,
     * so a blocking delay is fine here. */
    HAL_Delay(10);

    if (write_reg(hi2c, REG_CONFIG, CONFIG_DLPF_44HZ) != HAL_OK) {
        return HAL_ERROR;
    }
    if (write_reg(hi2c, REG_ACCEL_CONFIG, ACCEL_CONFIG_4G) != HAL_OK) {
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef MPU6050_ReadAccel(I2C_HandleTypeDef *hi2c, MPU6050_Accel *out)
{
    uint8_t buf[6];

    /* One burst read of X, Y and Z so all three axes come from the same
     * sample. Three separate reads could mix samples taken at different
     * times. */
    if (read_regs(hi2c, REG_ACCEL_XOUT_H, buf, sizeof(buf)) != HAL_OK) {
        return HAL_ERROR;
    }

    /* Each axis is big-endian, high byte first, two's complement. */
    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    out->ax_g = (float)raw_x / ACCEL_COUNTS_PER_G;
    out->ay_g = (float)raw_y / ACCEL_COUNTS_PER_G;
    out->az_g = (float)raw_z / ACCEL_COUNTS_PER_G;
    return HAL_OK;
}

float MPU6050_TiltDeg(const MPU6050_Accel *a)
{
    /* atan2 of horizontal over vertical instead of acos(az / |a|): it stays
     * accurate near 0 and 180 degrees and cannot be fed a ratio slightly
     * above 1 by noise. Only meaningful while the board is not accelerating. */
    float horizontal = sqrtf(a->ax_g * a->ax_g + a->ay_g * a->ay_g);
    return atan2f(horizontal, a->az_g) * RAD_TO_DEG;
}

float MPU6050_MagnitudeG(const MPU6050_Accel *a)
{
    return sqrtf(a->ax_g * a->ax_g + a->ay_g * a->ay_g + a->az_g * a->az_g);
}
