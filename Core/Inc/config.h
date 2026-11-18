#ifndef CONFIG_H
#define CONFIG_H

/*
 * All tunable numbers live here so I can change behaviour without touching
 * the drivers or main.c. These are starting values, not measured ones.
 */

/* Temperature warning with hysteresis. The red LED turns on at TEMP_WARN_ON_C
 * and only turns off again below TEMP_WARN_OFF_C, so a reading that sits right
 * at the limit does not make the LED flicker. */
#define TEMP_WARN_ON_C        45.0f
#define TEMP_WARN_OFF_C       42.0f

/* Angle between the board's Z axis and vertical above which the yellow LED
 * turns on. Assumes the MPU6050 is mounted flat with Z pointing up. */
#define TILT_WARN_DEG         30.0f

/* At rest the accelerometer reads about 1 g in total, so a magnitude well
 * above that means a knock or a drop. The accel range is set to +/-4 g so a
 * 2 g limit is not right at the edge of saturation. */
#define SHOCK_WARN_G          2.0f

/* A shock lasts a few milliseconds. Holding the LED on makes it visible. */
#define SHOCK_HOLD_MS         1000U

/* Number of ADC readings averaged per temperature update. */
#define LM35_SAMPLES          16U

/* Main loop task periods in milliseconds. */
#define HEARTBEAT_PERIOD_MS   500U
#define TEMP_PERIOD_MS        100U
#define IMU_PERIOD_MS         20U
#define LOG_PERIOD_MS         1000U

/* Set to 0 to build without UART output. */
#define UART_LOG_ENABLE       1

#endif /* CONFIG_H */
