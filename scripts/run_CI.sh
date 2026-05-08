#!/bin/bash

pio ci -l . --exclude=src/test --exclude=src/examples -O "framework=stm32cube" examples/timebase-example.c -b nucleo_h723zg
pio ci -l . --exclude=src/test --exclude=src/examples -O "framework=stm32cube" examples/tasks-example.c -b nucleo_h723zg