/*
 * Host tests for the firmware logic. These run on a PC against a fake HAL
 * (tests/stub), so they check my maths, register sequences and state
 * machines, not the real ADC, I2C bus or timing on the board.
 *
 * main.c is included directly so its static functions (update_temperature,
 * update_imu, update_leds, log_status) can be called. Its main() is renamed
 * so it does not clash with the test runner's.
 */

#include <math.h>
#include <stdio.h>
#include <string.h>

#include "hal_fake.h"

#define main firmware_main
#include "../Core/Src/main.c"
#undef main

/* ---- tiny test harness ---- */

static int checks_failed;
static int tests_run;
static int tests_failed;

#define CHECK(cond) do { \
    if (!(cond)) { \
        printf("    FAIL line %d: %s\n", __LINE__, #cond); \
        checks_failed++; \
    } \
} while (0)

#define CHECK_NEAR(a, b, tol) do { \
    double a_ = (double)(a), b_ = (double)(b); \
    if (fabs(a_ - b_) > (tol)) { \
        printf("    FAIL line %d: %s = %.4f, expected %.4f\n", \
               __LINE__, #a, a_, b_); \
        checks_failed++; \
    } \
} while (0)

#define RUN(fn) do { \
    int before_ = checks_failed; \
    fake_reset(); \
    reset_main_state(); \
    fn(); \
    tests_run++; \
    if (checks_failed != before_) { \
        tests_failed++; \
        printf("FAIL  %s\n", #fn); \
    } else { \
        printf("ok    %s\n", #fn); \
    } \
} while (0)

/* One LSB of the LM35 reading in degrees: 3300 mV / 4095 counts / 10 mV/C */
#define C_PER_COUNT (3300.0 / 4095.0 / 10.0)

/* Accelerometer counts per g at +/-4 g */
#define CPG 8192

static void reset_main_state(void)
{
    temp_c = 0.0f;
    tilt_deg = 0.0f;
    accel_mag_g = 0.0f;
    temp_warn = 0;
    tilt_warn = 0;
    shock_active = 0;
    shock_ms = 0;
    imu_ready = 1;
    imu_fault = 0;
}

/* Accel counts for a board tilted by deg from vertical, in the X-Z plane. */
static void set_tilt(double deg)
{
    double r = deg * 3.14159265358979 / 180.0;
    fake_mpu_set_accel((int16_t)lround(CPG * sin(r)), 0,
                       (int16_t)lround(CPG * cos(r)));
}

/* ================= LM35 driver ================= */

static void lm35_converts_room_temperature(void)
{
    float t = -1.0f;
    fake_adc_fill(310, LM35_SAMPLES);
    CHECK(LM35_ReadCelsius(&hadc1, &t) == HAL_OK);
    CHECK_NEAR(t, 310 * C_PER_COUNT, 0.001);   /* 24.98 C */
}

static void lm35_converts_zero_and_full_scale(void)
{
    float t = -1.0f;
    fake_adc_fill(0, LM35_SAMPLES);
    CHECK(LM35_ReadCelsius(&hadc1, &t) == HAL_OK);
    CHECK_NEAR(t, 0.0, 0.001);

    fake_adc_fill(4095, LM35_SAMPLES);
    CHECK(LM35_ReadCelsius(&hadc1, &t) == HAL_OK);
    CHECK_NEAR(t, 330.0, 0.001);
}

static void lm35_averages_all_samples(void)
{
    float t = -1.0f;
    /* Alternating 300 and 320 should average to exactly 310. */
    for (uint32_t i = 0; i < LM35_SAMPLES; i++) {
        fake.samples[i] = (i % 2) ? 320 : 300;
    }
    fake.n_samples = LM35_SAMPLES;
    CHECK(LM35_ReadCelsius(&hadc1, &t) == HAL_OK);
    CHECK_NEAR(t, 310 * C_PER_COUNT, 0.001);
    CHECK(fake.next_sample == LM35_SAMPLES);
    CHECK(fake.adc_starts == LM35_SAMPLES);
    CHECK(fake.adc_stops == 1);
}

static void lm35_timeout_returns_error_and_keeps_value(void)
{
    float t = 99.0f;
    fake_adc_fill(310, LM35_SAMPLES);
    fake.timeout_at = 4;
    CHECK(LM35_ReadCelsius(&hadc1, &t) == HAL_ERROR);
    CHECK_NEAR(t, 99.0, 0.0);
    CHECK(fake.adc_starts == 5);
    CHECK(fake.adc_stops == 1);   /* ADC is left stopped */
}

/* ================= MPU6050 driver ================= */

static void mpu_init_writes_expected_registers(void)
{
    fake.regs[0x75] = 0x68;
    uint32_t t0 = fake.tick;
    CHECK(MPU6050_Init(&hi2c1) == HAL_OK);
    CHECK(fake.last_read_reg == 0x75);
    CHECK(fake.n_writes == 3);
    CHECK(fake.writes[0].dev_addr == 0xD0);
    CHECK(fake.writes[0].reg == 0x6B && fake.writes[0].val == 0x01); /* wake, PLL clock */
    CHECK(fake.writes[1].reg == 0x1A && fake.writes[1].val == 0x03); /* DLPF 44 Hz */
    CHECK(fake.writes[2].reg == 0x1C && fake.writes[2].val == 0x08); /* +/-4 g */
    CHECK(fake.tick - t0 == 10);   /* settle delay after wake */
}

static void mpu_init_rejects_wrong_who_am_i(void)
{
    fake.regs[0x75] = 0x70;   /* MPU6500-style clone */
    CHECK(MPU6050_Init(&hi2c1) == HAL_ERROR);
    CHECK(fake.n_writes == 0);
}

static void mpu_init_fails_without_ack(void)
{
    fake.regs[0x75] = 0x68;
    fake.i2c_nack = 1;
    CHECK(MPU6050_Init(&hi2c1) == HAL_ERROR);
}

static void mpu_init_fails_at_wrong_address(void)
{
    /* AD0 pulled high puts the chip at 0x69; the driver only talks to 0x68. */
    fake.regs[0x75] = 0x68;
    fake.i2c_dev_addr = 0x69U << 1;
    CHECK(MPU6050_Init(&hi2c1) == HAL_ERROR);
}

static void mpu_read_decodes_signed_big_endian(void)
{
    MPU6050_Accel a;
    fake_mpu_set_accel(-CPG, 0, CPG);
    CHECK(MPU6050_ReadAccel(&hi2c1, &a) == HAL_OK);
    CHECK(fake.n_reads == 1);            /* one burst ... */
    CHECK(fake.last_read_reg == 0x3B);   /* ... from ACCEL_XOUT_H ... */
    CHECK(fake.last_read_len == 6);      /* ... of all six bytes */
    CHECK_NEAR(a.ax_g, -1.0, 1e-6);
    CHECK_NEAR(a.ay_g, 0.0, 1e-6);
    CHECK_NEAR(a.az_g, 1.0, 1e-6);
}

static void mpu_read_handles_full_scale(void)
{
    MPU6050_Accel a;
    fake_mpu_set_accel(32767, -32768, 0);
    CHECK(MPU6050_ReadAccel(&hi2c1, &a) == HAL_OK);
    CHECK_NEAR(a.ax_g, 32767.0 / CPG, 1e-6);   /* just under +4 g */
    CHECK_NEAR(a.ay_g, -4.0, 1e-6);
}

static void mpu_tilt_angles(void)
{
    MPU6050_Accel a;

    a = (MPU6050_Accel){ 0.0f, 0.0f, 1.0f };
    CHECK_NEAR(MPU6050_TiltDeg(&a), 0.0, 0.01);

    a = (MPU6050_Accel){ 0.5f, 0.0f, 0.8660254f };
    CHECK_NEAR(MPU6050_TiltDeg(&a), 30.0, 0.01);

    a = (MPU6050_Accel){ 0.5f, 0.5f, 0.7071068f };
    CHECK_NEAR(MPU6050_TiltDeg(&a), 45.0, 0.01);

    a = (MPU6050_Accel){ 1.0f, 0.0f, 0.0f };
    CHECK_NEAR(MPU6050_TiltDeg(&a), 90.0, 0.01);

    a = (MPU6050_Accel){ 0.0f, 0.0f, -1.0f };
    CHECK_NEAR(MPU6050_TiltDeg(&a), 180.0, 0.01);
}

static void mpu_magnitude(void)
{
    MPU6050_Accel a = { 0.0f, 0.0f, 1.0f };
    CHECK_NEAR(MPU6050_MagnitudeG(&a), 1.0, 1e-6);
    a = (MPU6050_Accel){ 1.2f, -1.6f, 0.0f };
    CHECK_NEAR(MPU6050_MagnitudeG(&a), 2.0, 1e-6);
}

/* ================= main.c logic ================= */

static void temp_warning_turns_on_at_45(void)
{
    fake_adc_fill(558, LM35_SAMPLES);   /* 44.97 C */
    update_temperature();
    CHECK(temp_warn == 0);

    fake_adc_fill(559, LM35_SAMPLES);   /* 45.05 C */
    update_temperature();
    CHECK(temp_warn == 1);
}

static void temp_warning_has_hysteresis(void)
{
    fake_adc_fill(600, LM35_SAMPLES);   /* 48.35 C */
    update_temperature();
    CHECK(temp_warn == 1);

    fake_adc_fill(540, LM35_SAMPLES);   /* 43.52 C: below 45, still on */
    update_temperature();
    CHECK(temp_warn == 1);

    fake_adc_fill(522, LM35_SAMPLES);   /* 42.07 C: still on */
    update_temperature();
    CHECK(temp_warn == 1);

    fake_adc_fill(521, LM35_SAMPLES);   /* 41.99 C: off */
    update_temperature();
    CHECK(temp_warn == 0);

    fake_adc_fill(540, LM35_SAMPLES);   /* back to 43.52 C: stays off */
    update_temperature();
    CHECK(temp_warn == 0);
}

static void temp_failed_read_keeps_last_value(void)
{
    fake_adc_fill(600, LM35_SAMPLES);
    update_temperature();
    float before = temp_c;

    fake_adc_fill(0, LM35_SAMPLES);
    fake.timeout_at = 0;
    update_temperature();
    CHECK_NEAR(temp_c, before, 0.0);
    CHECK(temp_warn == 1);
}

static void tilt_warning_threshold(void)
{
    set_tilt(0.0);
    update_imu(0);
    CHECK(tilt_warn == 0);
    CHECK_NEAR(accel_mag_g, 1.0, 0.001);

    set_tilt(29.0);
    update_imu(20);
    CHECK(tilt_warn == 0);

    set_tilt(31.0);
    update_imu(40);
    CHECK(tilt_warn == 1);

    set_tilt(180.0);
    update_imu(60);
    CHECK_NEAR(tilt_deg, 180.0, 0.05);
    CHECK(tilt_warn == 1);
}

static void shock_needs_more_than_2g(void)
{
    fake_mpu_set_accel(0, 0, 2 * CPG);   /* exactly 2.0 g */
    update_imu(0);
    CHECK(shock_active == 0);

    fake_mpu_set_accel(0, 0, 2 * CPG + 100);   /* 2.01 g */
    update_imu(20);
    CHECK(shock_active == 1);
}

static void shock_led_holds_for_one_second(void)
{
    fake_mpu_set_accel(0, 0, (int16_t)(2.5 * CPG));
    update_imu(1000);
    fake_mpu_set_accel(0, 0, CPG);   /* back at rest */
    update_imu(1020);

    update_leds(1999);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 1);
    update_leds(2000);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 0);
    CHECK(shock_active == 0);
}

static void shock_hold_survives_tick_wrap(void)
{
    uint32_t t = 0xFFFFFE00U;   /* 512 ms before the 32-bit tick wraps */
    fake_mpu_set_accel(0, 0, (int16_t)(2.5 * CPG));
    update_imu(t);

    /* Right after the shock, t + SHOCK_HOLD_MS has already wrapped but now
     * has not. A check written as now >= shock_ms + SHOCK_HOLD_MS would turn
     * the LED off here. */
    update_leds(t + 10U);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 1);
    update_leds(t + 999U);      /* wraps to 0x000001E7 */
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 1);
    update_leds(t + 1000U);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 0);
}

static void task_period_check_survives_tick_wrap(void)
{
    /* Same unsigned comparison the main loop uses for every task. */
    uint32_t last = 0xFFFFFF00U;
    uint32_t now  = last + HEARTBEAT_PERIOD_MS;        /* wraps */
    CHECK(now < last);
    CHECK(now - last >= HEARTBEAT_PERIOD_MS);
    CHECK(!((now - 1U) - last >= HEARTBEAT_PERIOD_MS));
}

static void imu_fault_blinks_yellow_at_5hz(void)
{
    imu_fault = 1;
    update_leds(0);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 0);
    update_leds(100);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 1);
    update_leds(199);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 1);
    update_leds(200);
    CHECK(fake_pin(LED_YELLOW_GPIO_Port, LED_YELLOW_Pin) == 0);
}

static void imu_read_failure_sets_fault_and_recovers(void)
{
    set_tilt(0.0);
    fake.i2c_nack = 1;
    update_imu(0);
    CHECK(imu_fault == 1);

    fake.i2c_nack = 0;
    update_imu(20);
    CHECK(imu_fault == 0);
}

static void imu_not_ready_is_never_read(void)
{
    imu_ready = 0;
    imu_fault = 1;
    set_tilt(45.0);
    update_imu(0);
    CHECK(fake.n_reads == 0);
    CHECK(imu_fault == 1);
    CHECK(tilt_warn == 0);
}

static void red_led_follows_temp_warning(void)
{
    temp_warn = 1;
    update_leds(0);
    CHECK(fake_pin(LED_RED_GPIO_Port, LED_RED_Pin) == 1);
    temp_warn = 0;
    update_leds(1);
    CHECK(fake_pin(LED_RED_GPIO_Port, LED_RED_Pin) == 0);
}

static void log_line_format(void)
{
    fake_adc_fill(310, LM35_SAMPLES);   /* 24.98 C */
    update_temperature();
    set_tilt(0.0);
    update_imu(0);
    log_status();
    CHECK(strcmp(fake.uart,
                 "T=24.9C tilt=0.0deg a=1.00g red=0 yel=0 imu=ok\r\n") == 0);
    if (strcmp(fake.uart,
               "T=24.9C tilt=0.0deg a=1.00g red=0 yel=0 imu=ok\r\n") != 0) {
        printf("    got: %s", fake.uart);
    }
}

static void log_line_shows_warnings_and_fault(void)
{
    temp_c = 46.25f;
    tilt_deg = 35.5f;
    accel_mag_g = 1.07f;
    temp_warn = 1;
    tilt_warn = 1;
    imu_fault = 1;
    log_status();
    CHECK(strcmp(fake.uart,
                 "T=46.2C tilt=35.5deg a=1.07g red=1 yel=1 imu=FAULT\r\n") == 0);
    if (strcmp(fake.uart,
               "T=46.2C tilt=35.5deg a=1.07g red=1 yel=1 imu=FAULT\r\n") != 0) {
        printf("    got: %s", fake.uart);
    }
}

int main(void)
{
    printf("LM35 driver\n");
    RUN(lm35_converts_room_temperature);
    RUN(lm35_converts_zero_and_full_scale);
    RUN(lm35_averages_all_samples);
    RUN(lm35_timeout_returns_error_and_keeps_value);

    printf("MPU6050 driver\n");
    RUN(mpu_init_writes_expected_registers);
    RUN(mpu_init_rejects_wrong_who_am_i);
    RUN(mpu_init_fails_without_ack);
    RUN(mpu_init_fails_at_wrong_address);
    RUN(mpu_read_decodes_signed_big_endian);
    RUN(mpu_read_handles_full_scale);
    RUN(mpu_tilt_angles);
    RUN(mpu_magnitude);

    printf("main.c logic\n");
    RUN(temp_warning_turns_on_at_45);
    RUN(temp_warning_has_hysteresis);
    RUN(temp_failed_read_keeps_last_value);
    RUN(tilt_warning_threshold);
    RUN(shock_needs_more_than_2g);
    RUN(shock_led_holds_for_one_second);
    RUN(shock_hold_survives_tick_wrap);
    RUN(task_period_check_survives_tick_wrap);
    RUN(imu_fault_blinks_yellow_at_5hz);
    RUN(imu_read_failure_sets_fault_and_recovers);
    RUN(imu_not_ready_is_never_read);
    RUN(red_led_follows_temp_warning);
    RUN(log_line_format);
    RUN(log_line_shows_warnings_and_fault);

    printf("\n%d tests, %d failed\n", tests_run, tests_failed);
    return tests_failed ? 1 : 0;
}
