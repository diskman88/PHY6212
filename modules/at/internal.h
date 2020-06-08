/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#ifndef AT2_INTERNAL_H
#define AT2_INTERNAL_H

#include <stdlib.h>

typedef struct {
    size_t buffer_size;
    size_t count;
    int max_size;
    char *line;
} linebuffer_t;

linebuffer_t *linebuffer_new(int max_size);
void linebuffer_free(linebuffer_t *line);
int linebuffer_putc(linebuffer_t *line, char c);
int linebuffer_cmp(linebuffer_t *line, char *s);

#endif