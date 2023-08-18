#pragma once

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>

#include "network.h"

#define NET_DATA_HEADER_SIZE sizeof(MessageHeader)
#define NET_DATA_BODY_MAX 500
#define NET_DATA_TOTAL_MAX NET_DATA_BODY_MAX + NET_DATA_HEADER_SIZE

#define GET_HEADER(data) ((MessageHeader *)data)
#define GET_BODY_SIZE(data) (((MessageHeader *)data)->size)
#define GET_TYPE(data) ((MessageHeader*) data)->type)
#define GET_BODY(data) ((void *)((uint8_t *)data + NET_DATA_HEADER_SIZE))

#define SET_HEADER(buff, htype, sz)                                                                \
    do {                                                                                           \
        GET_HEADER(buff)->size = sz;                                                               \
        GET_HEADER(buff)->type = htype;                                                            \
    } while (0)

#define SET_BODY(buff, data, len) memcpy(GET_BODY(buff), data, len)

#define SET_MESSAGE(buff, htype, sz, data)                                                         \
    do {                                                                                           \
        SET_HEADER(buff, htype, sz);                                                               \
        SET_BODY(buff, data, sz);                                                                  \
    } while (0)

#define SEND_MESSAGE(fd, buffer) write(fd, buffer, NET_DATA_HEADER_SIZE + GET_HEADER(buffer)->size)

#define SET_AND_SEND_HEADER(fd, buffer, type)                                                      \
    do {                                                                                           \
        SET_HEADER(buffer, type, 0);                                                               \
        SEND_MESSAGE(fd, buffer);                                                                  \
    } while (0)

#define SET_AND_SEND_MESSAGE(fd, buffer, type, size, data)                                         \
    do {                                                                                           \
        SET_HEADER(buffer, type, size);                                                            \
        SET_BODY(buffer, data, size);                                                              \
        SEND_MESSAGE(fd, buffer);                                                                  \
    } while (0);

typedef struct MessageHeader {
    int type;
    size_t size;
} MessageHeader;

typedef uint8_t MessageBuffer[NET_DATA_TOTAL_MAX + 1];

typedef struct MessageReader {
    int fd;
    MessageBuffer buff;
    size_t bytes_read;
    enum ReadingStatus {
        STATUS_READING_ERROR,
        STATUS_READING_HEADER,
        STATUS_READING_BODY,
        STATUS_READING_FINISHED,
    } status;
} MessageReader;

bool message_reader_init(MessageReader *rd, Socket *socket);

// Only call if init
ssize_t message_reader_process(MessageReader *rd);

void message_reader_rewind(MessageReader *rd);
