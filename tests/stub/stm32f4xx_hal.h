#ifndef STM32F4XX_HAL_STUB_H
#define STM32F4XX_HAL_STUB_H

/*
 * Minimal stand-in for the ST HAL so the firmware sources compile and run on
 * a PC. Only the types, constants and functions that my code and the
 * generated main.c use are here. Constant values do not match ST's; the tests
 * never depend on them, except the GPIO pin bits.
 */

#include <stdint.h>
#include <stddef.h>

typedef enum {
    HAL_OK      = 0x00U,
    HAL_ERROR   = 0x01U,
    HAL_BUSY    = 0x02U,
    HAL_TIMEOUT = 0x03U
} HAL_StatusTypeDef;

#define DISABLE 0U
#define ENABLE  1U

/* ---- GPIO ---- */
typedef struct {
    uint32_t odr;   /* output state, one bit per pin */
} GPIO_TypeDef;

extern GPIO_TypeDef stub_gpioa, stub_gpiob, stub_gpioc, stub_gpioh;
#define GPIOA (&stub_gpioa)
#define GPIOB (&stub_gpiob)
#define GPIOC (&stub_gpioc)
#define GPIOH (&stub_gpioh)

#define GPIO_PIN_0  ((uint16_t)0x0001)
#define GPIO_PIN_1  ((uint16_t)0x0002)
#define GPIO_PIN_13 ((uint16_t)0x2000)

typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET } GPIO_PinState;

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
} GPIO_InitTypeDef;

#define GPIO_MODE_OUTPUT_PP   1U
#define GPIO_NOPULL           0U
#define GPIO_SPEED_FREQ_LOW   0U

/* ---- ADC ---- */
typedef struct {
    uint32_t ClockPrescaler;
    uint32_t Resolution;
    uint32_t ScanConvMode;
    uint32_t ContinuousConvMode;
    uint32_t DiscontinuousConvMode;
    uint32_t ExternalTrigConvEdge;
    uint32_t ExternalTrigConv;
    uint32_t DataAlign;
    uint32_t NbrOfConversion;
    uint32_t DMAContinuousRequests;
    uint32_t EOCSelection;
} ADC_InitTypeDef;

typedef struct {
    void *Instance;
    ADC_InitTypeDef Init;
} ADC_HandleTypeDef;

typedef struct {
    uint32_t Channel;
    uint32_t Rank;
    uint32_t SamplingTime;
} ADC_ChannelConfTypeDef;

#define ADC1                          ((void *)0x1)
#define ADC_CLOCK_SYNC_PCLK_DIV4      0U
#define ADC_RESOLUTION_12B            0U
#define ADC_EXTERNALTRIGCONVEDGE_NONE 0U
#define ADC_SOFTWARE_START            0U
#define ADC_DATAALIGN_RIGHT           0U
#define ADC_EOC_SINGLE_CONV           0U
#define ADC_CHANNEL_0                 0U
#define ADC_SAMPLETIME_480CYCLES      7U

/* ---- I2C ---- */
typedef struct {
    uint32_t ClockSpeed;
    uint32_t DutyCycle;
    uint32_t OwnAddress1;
    uint32_t AddressingMode;
    uint32_t DualAddressMode;
    uint32_t OwnAddress2;
    uint32_t GeneralCallMode;
    uint32_t NoStretchMode;
} I2C_InitTypeDef;

typedef struct {
    void *Instance;
    I2C_InitTypeDef Init;
} I2C_HandleTypeDef;

#define I2C1                     ((void *)0x2)
#define I2C_DUTYCYCLE_2          0U
#define I2C_ADDRESSINGMODE_7BIT  0U
#define I2C_DUALADDRESS_DISABLE  0U
#define I2C_GENERALCALL_DISABLE  0U
#define I2C_NOSTRETCH_DISABLE    0U
#define I2C_MEMADD_SIZE_8BIT     1U

/* ---- UART ---- */
typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct {
    void *Instance;
    UART_InitTypeDef Init;
} UART_HandleTypeDef;

#define USART1                ((void *)0x3)
#define UART_WORDLENGTH_8B    0U
#define UART_STOPBITS_1       0U
#define UART_PARITY_NONE      0U
#define UART_MODE_TX_RX       0U
#define UART_HWCONTROL_NONE   0U
#define UART_OVERSAMPLING_16  0U

/* ---- RCC, PWR, FLASH ---- */
typedef struct {
    uint32_t PLLState;
    uint32_t PLLSource;
    uint32_t PLLM;
    uint32_t PLLN;
    uint32_t PLLP;
    uint32_t PLLQ;
} RCC_PLLInitTypeDef;

typedef struct {
    uint32_t OscillatorType;
    uint32_t HSEState;
    RCC_PLLInitTypeDef PLL;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

#define RCC_OSCILLATORTYPE_HSE   0U
#define RCC_HSE_ON               0U
#define RCC_PLL_ON               0U
#define RCC_PLLSOURCE_HSE        0U
#define RCC_PLLP_DIV2            0U
#define RCC_CLOCKTYPE_HCLK       0U
#define RCC_CLOCKTYPE_SYSCLK     0U
#define RCC_CLOCKTYPE_PCLK1      0U
#define RCC_CLOCKTYPE_PCLK2      0U
#define RCC_SYSCLKSOURCE_PLLCLK  0U
#define RCC_SYSCLK_DIV1          0U
#define RCC_HCLK_DIV1            0U
#define RCC_HCLK_DIV2            0U
#define FLASH_LATENCY_2          0U
#define PWR_REGULATOR_VOLTAGE_SCALE2 0U

#define __HAL_RCC_PWR_CLK_ENABLE()          do { } while (0)
#define __HAL_RCC_GPIOA_CLK_ENABLE()        do { } while (0)
#define __HAL_RCC_GPIOB_CLK_ENABLE()        do { } while (0)
#define __HAL_RCC_GPIOC_CLK_ENABLE()        do { } while (0)
#define __HAL_RCC_GPIOH_CLK_ENABLE()        do { } while (0)
#define __HAL_PWR_VOLTAGESCALING_CONFIG(x)  do { (void)(x); } while (0)

/* ---- functions ---- */
HAL_StatusTypeDef HAL_Init(void);
uint32_t HAL_GetTick(void);
void HAL_Delay(uint32_t Delay);
void __disable_irq(void);

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency);

void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);

HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef *hadc, ADC_ChannelConfTypeDef *sConfig);
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef *hadc);
HAL_StatusTypeDef HAL_ADC_PollForConversion(ADC_HandleTypeDef *hadc, uint32_t Timeout);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);

HAL_StatusTypeDef HAL_I2C_Init(I2C_HandleTypeDef *hi2c);
HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                    uint16_t MemAddress, uint16_t MemAddSize,
                                    uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef *hi2c, uint16_t DevAddress,
                                   uint16_t MemAddress, uint16_t MemAddSize,
                                   uint8_t *pData, uint16_t Size, uint32_t Timeout);

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, const uint8_t *pData,
                                    uint16_t Size, uint32_t Timeout);

#endif /* STM32F4XX_HAL_STUB_H */
