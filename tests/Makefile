# Host tests: build and run on a PC with gcc.
#   make -C tests
# CubeIDE only compiles Core/ and Drivers/, so nothing here ends up in the
# firmware.

CC      ?= gcc
CFLAGS  := -std=c11 -Wall -Wextra -O1 -g
INCLUDE := -Istub -I../Core/Inc
BUILD   := build

SRCS := test_firmware.c stub/hal_stub.c ../Core/Src/lm35.c ../Core/Src/mpu6050.c

.PHONY: all test clean

all: test

$(BUILD)/test_firmware: $(SRCS) stub/stm32f4xx_hal.h stub/hal_fake.h \
                        ../Core/Src/main.c ../Core/Inc/config.h \
                        ../Core/Inc/lm35.h ../Core/Inc/mpu6050.h
	@mkdir -p $(BUILD)
	$(CC) $(CFLAGS) $(INCLUDE) $(SRCS) -lm -o $@

test: $(BUILD)/test_firmware
	./$(BUILD)/test_firmware

clean:
	rm -rf $(BUILD)
