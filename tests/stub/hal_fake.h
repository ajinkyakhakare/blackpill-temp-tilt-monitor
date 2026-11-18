#ifndef HAL_FAKE_H
#define HAL_FAKE_H

/*
 * Controls for the fake HAL in hal_stub.c. A test sets up what the "hardware"
 * should return, calls the firmware, then checks what the firmware did.
 */

#include "stm32f4xx_hal.h"

#define FAKE_ADC_MAX_SAMPLES  64U
#define FAKE_I2C_MAX_WRITES   16U
#define FAKE_UART_BUF_SIZE    512U

typedef struct {
    uint16_t dev_addr;
    uint8_t  reg;
    uint8_t  val;
} FakeI2CWrite;

typedef struct {
    /* time */
    uint32_t tick;

    /* ADC: conversions return samples[] in order. timeout_at is the index of
     * the conversion that times out; -1 means none does. */
    uint32_t samples[FAKE_ADC_MAX_SAMPLES];
    uint32_t n_samples;
    uint32_t next_sample;
    int      timeout_at;
    uint32_t adc_starts;
    uint32_t adc_stops;

    /* I2C: a 128-byte register map behind one device address. */
    uint16_t     i2c_dev_addr;   /* 8-bit form, as HAL expects it */
    int          i2c_nack;       /* 1: every transfer fails */
    uint8_t      regs[128];
    FakeI2CWrite writes[FAKE_I2C_MAX_WRITES];
    uint32_t     n_writes;
    uint32_t     n_reads;
    uint8_t      last_read_reg;
    uint16_t     last_read_len;

    /* UART: everything transmitted, as one string */
    char     uart[FAKE_UART_BUF_SIZE];
    uint32_t uart_len;
} HalFake;

extern HalFake fake;

/* Clears all fake state and GPIO outputs. */
void fake_reset(void);

/* Queues n ADC conversions that all return the same value. */
void fake_adc_fill(uint32_t value, uint32_t n);

/* Puts a signed 16-bit accelerometer reading for each axis into the
 * MPU6050 register map (ACCEL_XOUT_H at 0x3B, big-endian). */
void fake_mpu_set_accel(int16_t x, int16_t y, int16_t z);

/* 1 if the pin's output bit is set. */
int fake_pin(GPIO_TypeDef *port, uint16_t pin);

#endif /* HAL_FAKE_H */
