#include "messages.h"
#include "common.h"
#include "network.h"

void message_reader_rewind(MessageReader *rd)
{
    rd->status = STATUS_READING_HEADER;
    rd->bytes_read = 0;
}

bool message_reader_init(MessageReader *rd, Socket *socket)
{
    if (socket->status != SOCKET_CLIENT)
        return false;

    rd->bytes_read = 0;
    rd->fd = socket->fd;
    rd->status = STATUS_READING_HEADER;
    return true;
}

ssize_t message_reader_process(MessageReader *rd)
{
    if (rd->status == STATUS_READING_FINISHED)
        return -1;

    if (rd->bytes_read == NET_DATA_TOTAL_MAX)
        return -1;

    int n;
    int fd = rd->fd;
    uint8_t *read_buff = (uint8_t *)rd->buff + rd->bytes_read;
    size_t message_left = sizeof(MessageHeader);
    size_t bytes_read = 0;

    if (rd->status == STATUS_READING_BODY)
        message_left += GET_HEADER(rd->buff)->size;

    message_left -= rd->bytes_read;

    do {
        size_t buff_left = NET_DATA_TOTAL_MAX - rd->bytes_read;
        size_t nmax = MIN(message_left, buff_left);

        n = read(fd, read_buff, nmax);

        if (n > 0) {
            rd->bytes_read += n;
            read_buff += n;
            message_left -= n;
            bytes_read += n;

        }

        if (message_left == 0) {
            if (++rd->status == STATUS_READING_BODY)
                message_left = GET_HEADER(rd->buff)->size;
        }
    } while (n > 0 && rd->status != STATUS_READING_FINISHED && rd->bytes_read < NET_DATA_TOTAL_MAX);

    return bytes_read;
}
