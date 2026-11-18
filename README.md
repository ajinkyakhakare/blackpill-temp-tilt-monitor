# blackpill-temp-tilt-monitor

This is a small monitoring node on an STM32F401 Black Pill. It reads
temperature from an LM35 and orientation from an MPU6050, and drives two
warning LEDs: red when the board gets too hot, yellow when it is tilted too
far or takes a knock. The onboard LED blinks as a heartbeat, and a status line
goes out over UART once a second.

I wrote it to practise STM32 HAL firmware end to end: ADC, I2C and UART on real
parts, small separate drivers, and a main loop that schedules everything with
`HAL_GetTick()` instead of blocking delays.

**Status:** the firmware is written and its logic passes host tests on a PC
(see "Testing"), but it has not been flashed or tested on hardware yet. The
measured results under "Testing" are placeholders until I do.

## Hardware

The board is a WeAct Black Pill with an STM32F401CCU6
and a 25 MHz crystal.
Temperature comes from an LM35 and
acceleration from a GY-521 MPU6050 breakout. The warning LEDs are a red and a
yellow 5 mm LED, each with a 330 ohm series resistor. I flash and debug with an
ST-Link V2 and read the UART log with a USB-TTL adapter.

## Wiring

| From | To | Notes |
|------|----|-------|
| LM35 +Vs | 5V | Only live when USB-C is plugged in, see docs/wiring.md |
| LM35 Vout | PA0 | ADC1 channel 0 |
| LM35 GND | GND | |
| MPU6050 VCC | 3V3 | GY-521 has its own regulator and pull-ups |
| MPU6050 GND | GND | |
| MPU6050 SCL | PB6 | I2C1 |
| MPU6050 SDA | PB7 | I2C1 |
| MPU6050 AD0 | not connected | Pulled low on the GY-521, address 0x68 |
| PB0 | 330 ohm, red LED anode | LED cathode to GND |
| PB1 | 330 ohm, yellow LED anode | LED cathode to GND |
| PA9 | USB-TTL RX | Adapter GND to board GND |
| ST-Link SWDIO, SWCLK, GND | SWDIO, SWCLK, GND | 3.3 V left off when USB-C powers the board |


## Pin map

| Pin | Function | CubeMX setting |
|-----|----------|----------------|
| PA0 | LM35 input | ADC1_IN0 |
| PB6 | I2C1 SCL | I2C1, standard mode 100 kHz |
| PB7 | I2C1 SDA | I2C1 |
| PB0 | Red LED | GPIO output, label LED_RED |
| PB1 | Yellow LED | GPIO output, label LED_YELLOW |
| PC13 | Onboard LED, active low | GPIO output, label LED_HEARTBEAT, initial level high |
| PA9 | USART1 TX | USART1 asynchronous, 115200 8N1 |
| PA13, PA14 | SWDIO, SWCLK | SYS debug: Serial Wire |
| PH0, PH1 | 25 MHz crystal | RCC HSE: crystal/ceramic resonator |

## Building and flashing

I generated the project in STM32CubeIDE (File, New, STM32 Project, target
STM32F401CCUx) and set it up in the .ioc editor like this. Under System Core I
set RCC HSE to crystal and SYS debug to Serial Wire. In Clock Configuration I
set the HSE input to 25 MHz, chose the PLL as system clock and typed 84 into
HCLK so CubeMX works out the dividers (APB1 ends up at 42 MHz, APB2 at 84 MHz).
ADC1 has IN0 enabled at 12-bit resolution, with single conversion,
continuous mode off and a 480-cycle sampling time on channel 0. The long
sampling time gives the ADC's sample capacitor plenty of time to charge from
the LM35 output. I2C1 runs in standard mode at 100 kHz on PB6 and PB7, USART1
is asynchronous at 115200 8N1, and the GPIO pins have the labels from the
table above. The firmware refers to them by label.

My own code lives in `Core/Inc` and `Core/Src` (`config.h`, `lm35.c/h`,
`mpu6050.c/h`) and in the USER CODE blocks of `main.c`, so regenerating code
from the .ioc does not overwrite it. CubeIDE already compiles those folders,
so no build settings need changing.

To flash, I connect the ST-Link, build with Project, Build Project, then use
Run, Debug As, STM32 C/C++ Application. The default ST-Link GDB server settings
work for SWD. To see the log, open a serial terminal on the USB-TTL adapter's
port at 115200 baud.

If the linker complains about `atan2f` or `sqrtf`, the math library is
missing: add `m` under Project Properties, C/C++ Build, Settings, MCU GCC
Linker, Libraries.

## How it works

After the CubeMX init, `MPU6050_Init()` reads WHO_AM_I and stops if the answer
is not 0x68. That way a wrong chip or bad wiring shows up straight away, rather
than as believable but wrong numbers later. It then clears the sleep bit
(the chip powers up asleep), turns on the 44 Hz digital low-pass filter and
sets the accelerometer to +/-4 g. The default +/-2 g would clip right at the
shock limit.

The main loop has four tasks, each with its own period: the heartbeat every
500 ms, temperature every 100 ms, the accelerometer every 20 ms and the UART
log every second. Each task stores the tick of its last run and checks
`now - last >= period`. Because that subtraction is unsigned, it keeps working
when the 32-bit tick counter wraps after about 49 days.

For temperature I take 16 ADC conversions, average them, and convert with
10 mV per degree against a 3.3 V reference. The red LED uses two thresholds:
it turns on at 45 °C and only turns off at 42 °C or lower, so it does not flicker
when the temperature sits near the limit.

For orientation I read all six accel bytes in one I2C burst, so X, Y and Z
come from the same sample. Tilt is `atan2(sqrt(ax² + ay²), az)`, the angle
between the board's Z axis and gravity. I use atan2 rather than acos because it
stays accurate near 0° and 180° and can't be fed an out-of-range value by
noise. A shock is any reading where the total acceleration goes above 2 g. The
yellow LED is solid for tilt, stays on for one second after a shock, and blinks
fast if the MPU6050 failed to start or a read is failing.

## Thresholds

All of these are in `Core/Inc/config.h`. They are starting values I picked, not
tuned ones.

| Setting | Value |
|---------|-------|
| Temperature warning on | 45.0 °C |
| Temperature warning off | 42.0 °C |
| Tilt warning | 30° from vertical |
| Shock warning | 2.0 g total |
| Shock LED hold time | 1000 ms |
| LM35 samples per reading | 16 |

## Testing

### Host tests

The `tests/` folder builds my drivers and the USER CODE in `main.c` on a PC,
against a small fake HAL that stands in for the ADC, I2C, GPIO, UART and the
tick counter. CubeIDE only compiles `Core/` and `Drivers/`, so none of this
ends up in the firmware. To run them:

```
make -C tests
```

The 26 tests cover the LM35 conversion and averaging, an ADC timeout, the
MPU6050 WHO_AM_I check and register setup, signed big-endian decoding, the
tilt and magnitude maths, the temperature hysteresis, the tilt and shock
thresholds, the 1 s shock hold, the 5 Hz fault blink, recovery after a failed
I2C read, the tick counter wrapping, and the exact UART line format. All 26
pass.

To make sure the tests can actually fail, the code was broken on purpose in a
few ways: wrong accel range, unshifted I2C address, swapped byte order, no
hysteresis, wrong averaging divisor, and a shock hold written as
`now >= shock_ms + SHOCK_HOLD_MS`. Each one makes at least one test fail. The
last one only fails because of a check a few milliseconds after a shock that
happens just before the tick counter wraps.

These tests check logic only. They can't show ADC accuracy, noise, I2C
timing, wiring or power problems, or whether the generated HAL setup is
right. That needs the board.


## Known limitations

The temperature is only as accurate as the 3.3 V rail, because VDDA is the ADC
reference and I don't measure it. The firmware could read the internal VREFINT
channel and correct for this, but it doesn't yet. The basic LM35 circuit also
can't read below about 2 °C, and the LM35 needs USB power to work at all.

Tilt is only meaningful while the board is still. Any movement adds to the
gravity vector, so the tilt reading during a knock is wrong. That's acceptable
here because the yellow LED is on for the shock anyway. Tilt also assumes the
MPU6050 is mounted flat with Z up. There is no calibration for mounting angle
or accelerometer offset.

The WHO_AM_I check is strict. Some cheap GY-521 boards carry an MPU6500 or a
clone that returns a different ID, and those will be reported as a fault even
if they would otherwise work.

If the I2C bus gets stuck (for example SDA held low after a wire is pulled
mid-transfer), the STM32F4 I2C peripheral can stay busy. The firmware flags
the fault but does not try to recover the bus, so a reset may be needed.

The ADC reads and the UART transmit are blocking. They take well under 10 ms
per pass, which is fine at these rates, but it would need DMA or interrupts if
the loop had to do more.

PA0 is shared with the Black Pill's KEY button, so pressing KEY drags the
temperature reading to about 0 °C.

