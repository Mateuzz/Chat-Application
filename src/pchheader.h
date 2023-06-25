#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>

#define SIZE_ARRAY(a) (sizeof(a) / sizeof(a[0]))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef DEBUG
    #define PRINT_DEBUG(...) printf(__VA_ARGS__)
#else
    #define PRINT_DEBUG(...)
#endif

#define CLOCK_TO_MS(c) ((c)*1000)
#define CLOCK_TO_SECONDS(c) ((c)*1000000)
