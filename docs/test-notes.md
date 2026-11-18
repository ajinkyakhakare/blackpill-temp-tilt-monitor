# Test notes

Nothing in this file has been measured on the board yet. The "Expected"
values are worked out from the datasheets and the code, not measured. I'll
fill each TODO in the Result column after flashing and testing on the real
board.

The logic itself is checked by host tests that run on a PC, see
"Host tests" at the end.

## Setup

TODO: date tested, board revision, ST-Link and USB-TTL adapter used,
supply voltage measured on 3V3 and 5V pins.

## Startup

| Check | Expected | Result |
|-------|----------|--------|
| Heartbeat on PC13 | toggles every 500 ms, so one full blink per second | TODO |
| UART banner at 115200 8N1 | `blackpill-temp-tilt-monitor start` printed once | TODO |
| Status line rate | one line per second, about 4.2 ms on the wire each | TODO |
| MPU6050 present | yellow LED off, log shows imu=ok | TODO |
| MPU6050 unplugged at boot | yellow LED blinks at 5 Hz, log shows `MPU6050 init failed` | TODO |

## Temperature

| Check | Expected | Result |
|-------|----------|--------|
| Room temperature vs reference thermometer | within about 1 °C; at 25 °C the LM35 gives 250 mV, about 310 ADC counts | TODO |
| Heat LM35 above 45 °C | red LED on from 45.05 °C (559 counts) | TODO |
| Let it cool | red LED stays on down to 42.07 °C, off at 41.99 °C (521 counts) | TODO |
| Reading noise at rest | moves in steps of 0.08 °C (one ADC count); spread TODO over 1 minute | TODO |

## Tilt and shock

| Check | Expected | Result |
|-------|----------|--------|
| Board flat | Z about +8192 counts, tilt near 0 deg, a near 1.00 g | TODO |
| Tilt to about 45 deg | X and Z about 5793 counts each, yellow LED on | TODO |
| Board upside down | Z about -8192 counts, tilt near 180 deg | TODO |
| Sharp tap on the table | total above 2.0 g (16384 counts), yellow LED on for 1 s | TODO |
| Pull SDA wire while running | yellow LED blinks at 5 Hz, log shows imu=FAULT; note whether it recovers without a reset | TODO |

## Calculated reference values

These come from the datasheets and the settings in the code. They are what
the readings should be if everything is ideal, not what I measured.

| Quantity | Value | How I got it |
|----------|-------|--------------|
| LM35 output | 10 mV per °C | LM35 datasheet |
| ADC step | 0.806 mV, 0.081 °C | 3300 mV / 4095 counts |
| 25 °C | 250 mV, 310.2 counts, read as 24.98 °C | 250 / 0.806 |
| Red LED on | 450 mV, 558.4 counts | 45 °C threshold |
| Red LED off | 420 mV, 521.2 counts | 42 °C threshold |
| Effect of 3V3 rail error | 1 % rail error gives 1 % reading error, 0.25 °C at 25 °C | VDDA is the ADC reference |
| One ADC conversion | 23.4 µs | (480 + 12) cycles at 21 MHz (84 MHz APB2 / 4) |
| One temperature reading | about 0.37 ms plus HAL overhead | 16 conversions |
| Accelerometer scale | 8192 counts per g | ±4 g range, MPU6050 datasheet |
| 30° tilt threshold | X 4096, Z 7094 counts | 8192 × sin and cos of 30° |
| 6-byte I2C burst read | about 0.9 ms | about 93 bit times at 100 kHz |
| Status line | 48 characters, about 4.2 ms | 10 bits per character at 115200 baud |
| LED current | about 3.9 mA | (3.3 V - 2.0 V) / 330 ohm, LED drop assumed 2.0 V |

The log prints one decimal place by cutting off the rest rather than
rounding, so 24.98 °C shows as `T=24.9C`.

## Host tests

`tests/` builds the firmware sources on a PC against a fake HAL and checks
the parts that don't need the board: the LM35 conversion and averaging, the
MPU6050 register setup and byte decoding, tilt and magnitude maths, the
temperature hysteresis, tilt and shock thresholds, the 1 s shock hold, the
fault blink, recovery after a failed I2C read, tick wrap-around and the exact
UART line format. Run them with `make -C tests`.

They can't catch anything that depends on the real hardware: ADC accuracy
and noise, I2C timing and clock stretching, clone chips, wiring, power, or
the HAL setup in the generated code.

Current result: 26 tests, 0 failed.

## Bugs found

TODO
