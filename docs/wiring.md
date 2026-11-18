# Wiring notes

## Power

The board is powered from its USB-C port during normal use. That matters for
the LM35: the Black Pill's 5V pin is connected straight to USB VBUS, so it only
has 5 V on it when USB is plugged in. The LM35 needs at least 4 V. If the board
is powered only from the ST-Link's 3.3 V pin, the 5V pin is dead and the
temperature reading is meaningless.

When USB-C is plugged in I connect only SWDIO, SWCLK and GND from the ST-Link,
and leave its 3.3 V pin off so two supplies are not tied together.

The MPU6050 breakout runs from the 3V3 pin. The GY-521 has its own regulator
and I2C pull-ups, so no extra resistors are needed on SCL and SDA.

## PA0 and the KEY button

On the Black Pill, PA0 is also wired to the onboard KEY button, which pulls it
to GND when pressed. I kept the LM35 on PA0 because it is the ADC1 channel 0
pin, but pressing KEY while the firmware runs will pull the reading to about
0 °C. Don't press it.

## LEDs

Both warning LEDs are wired active high: GPIO pin, then a series resistor,
then the LED anode, cathode to GND. At 3.3 V with a red or yellow LED (about
2 V forward drop), 330 ohm gives roughly 4 mA, which is well inside what a GPIO
pin can source and bright enough to see.

The onboard blue LED on PC13 is active low, so the firmware's PC13 output is
initialised high to keep it off at boot. PC13 can only sink a few mA, which is
fine for the onboard LED but not for anything bigger.

## UART

Only the adapter's RX goes to PA9 (USART1 TX). PA10 is configured as RX by
CubeMX but nothing is connected to it. The adapter's GND must go to board GND.
I don't connect the adapter's VCC pin.

## Photos

TODO: photo of the breadboard, top view
TODO: close-up of the MPU6050 and LM35 connections
