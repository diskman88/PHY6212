/*
 * Copyright (C) 2015-2017 Alibaba Group Holding Limited
 */

#ifndef AOS_TYPES_H
#define AOS_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#define POLLIN  0x1
#define POLLOUT 0x2
#define POLLERR 0x4
struct pollfd {
    int fd;
    short events;
    short revents;
};

#define AOS_EXPORT(...)

#endif /* AOS_TYPES_H */

