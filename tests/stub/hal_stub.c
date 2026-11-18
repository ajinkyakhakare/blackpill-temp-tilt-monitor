#include <string.h>
#include "hal_fake.h"

HalFake fake;
GPIO_TypeDef stub_gpioa, stub_gpiob, stub_gpioc, stub_gpioh;

void fake_reset(void)
{
    memset(&fake, 0, sizeof(fake));
    fake.timeout_at = -1;
    fake.i2c_dev_addr = 0x68U << 1;
    stub_gpioa.odr = stub_gpiob.odr = stub_gpioc.odr = stub_gpioh.odr = 0;
}

void fake_adc_fill(uint32_t value, uint32_t n)
{
    for (uint32_t i = 0; i < n && i < FAKE_ADC_MAX_SAMPLES; i++) {
        fake.samples[i] = value;
    }
    fake.n_samples = n;
    fake.next_sample = 0;
}

void fake_mpu_set_accel(int16_t x, int16_t y, int16_t z)
{
    int16_t v[3] = { x, y, z };
    for (int i = 0; i < 3; i++) {
        uint16_t u = (uint16_t)v[i];
        fake.regs[0x3B + 2 * i]     = (uint8_t)(u >> 8);
        fake.regs[0x3B + 2 * i + 1] = (uint8_t)(u & 0xFFU);
    }
}

int fake_pin(GPIO_TypeDef *port, uint16_t pin)
{
    return (port->odr & pin) != 0;
}

/* ---- time and core ---- */
HAL_StatusTypeDef HAL_Init(void) { return HAL_OK; }
uint32_t HAL_GetTick(void) { return fake.tick; }
void HAL_Delay(uint32_t Delay) { fake.tick += Delay; }
void __disable_irq(void) { }

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *p) { (void)p; return HAL_OK; }
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *p, uint32_t l) { (void)p; (void)l; return HAL_OK; }

/* ---- GPIO ---- */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init) { (void)GPIOx; (void)GPIO_Init; }

void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    if (PinState == GPIO_PIN_SET) {
        GPIOx->odr |= GPIO_Pin;
    } else {
        GPIOx->odr &= ~(uint32_t)GPIO_Pin;
    }
}

void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->odr ^= GPIO_Pin;
}

/* ---- ADC ---- */
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *h) { (void)h; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *h, ADC_ChannelConfTypeDef *c) { (void)h; (void)c; return HAL_OK; }

HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *h) { (void)h; fake.adc_starts++; return HAL_OK; }
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *h) { (void)h; fake.adc_stops++; return HAL_OK; }

HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *h, uint32_t Timeout)
{
    (void)h; (void)Timeout;
    if (fake.timeout_at >= 0 && fake.next_sample == (uint32_t)fake.timeout_at) {
        return HAL_TIMEOUT;
    }
    if (fake.next_sample >= fake.n_samples) {
        return HAL_TIMEOUT;   /* test did not queue enough samples */
    }
    return HAL_OK;
}

uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *h)
{
    (void)h;
    return fake.samples[fake.next_sample++];
}

/* ---- I2C ---- */
HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *h) { (void)h; return HAL_OK; }

HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)hi2c; (void)MemAddSize; (void)Timeout;
    if (fake.i2c_nack || DevAddress != fake.i2c_dev_addr) {
        return HAL_ERROR;
    }
    for (uint16_t i = 0; i < Size; i++) {
        uint8_t reg = (uint8_t)(MemAddress + i);
        fake.regs[reg & 0x7FU] = pData[i];
        if (fake.n_writes < FAKE_I2C_MAX_WRITES) {
            fake.writes[fake.n_writes].dev_addr = DevAddress;
            fake.writes[fake.n_writes].reg = reg;
            fake.writes[fake.n_writes].val = pData[i];
            fake.n_writes++;
        }
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
    (void)hi2c; (void)MemAddSize; (void)Timeout;
    if (fake.i2c_nack || DevAddress != fake.i2c_dev_addr) {
        return HAL_ERROR;
    }
    fake.n_reads++;
    fake.last_read_reg = (uint8_t)MemAddress;
    fake.last_read_len = Size;
    for (uint16_t i = 0; i < Size; i++) {
        pData[i] = fake.regs[(MemAddress + i) & 0x7FU];
    }
    return HAL_OK;
}

/* ---- UART ---- */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *h) { (void)h; return HAL_OK; }

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout)
{
    (void)huart; (void)Timeout;
    for (uint16_t i = 0; i < Size && fake.uart_len < FAKE_UART_BUF_SIZE - 1; i++) {
        fake.uart[fake.uart_len++] = (char)pData[i];
    }
    fake.uart[fake.uart_len] = '\0';
    return HAL_OK;
}
