#!/bin/bash

make rpi_arm64_defconfig CROSS_COMPILE=aarch64-linux-gnu-
make CROSS_COMPILE=aarch64-linux-gnu-
