#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdbool.h>

#ifndef F_CPU
#define F_CPU 1000000L // 1 MHz clock speed
#endif
#include <util/delay.h>
