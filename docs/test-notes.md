# Test notes

Nothing in this file has been measured yet. I'll fill each TODO in after
flashing and testing on the real board.

## Setup

TODO: date tested, board revision, ST-Link and USB-TTL adapter used,
supply voltage measured on 3V3 and 5V pins.

## Startup

| Check | Expected | Result |
|-------|----------|--------|
| Heartbeat on PC13 | toggles every 500 ms | TODO |
| UART banner at 115200 8N1 | start line printed once | TODO |
| MPU6050 present | yellow LED off, log shows imu=ok | TODO |
| MPU6050 unplugged at boot | yellow LED blinks fast, log shows init failed | TODO |

## Temperature

| Check | Expected | Result |
|-------|----------|--------|
| Room temperature vs reference thermometer | within about 1 °C | TODO |
| Heat LM35 above 45 °C | red LED on | TODO |
| Let it cool | red LED off only below 42 °C | TODO |
| Reading noise at rest | TODO: spread over 1 minute | TODO |

## Tilt and shock

| Check | Expected | Result |
|-------|----------|--------|
| Board flat | tilt near 0 deg, a near 1.00 g | TODO |
| Tilt to about 45 deg | yellow LED on | TODO |
| Board upside down | tilt near 180 deg | TODO |
| Sharp tap on the table | yellow LED on for about 1 s | TODO |
| Pull SDA wire while running | yellow LED blinks fast; note whether it recovers without a reset | TODO |

## Bugs found

TODO
