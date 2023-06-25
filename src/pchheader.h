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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <netdb.h>

#include <fcntl.h>

#define SIZE_ARRAY(a) (sizeof(a) / sizeof(a[0]))

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

#ifdef DEBUG
    #define PRINT_DEBUG(...) printf(__VA_ARGS__)
#else
    #define PRINT_DEBUG(...)
#endif
