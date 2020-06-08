/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */

#include <stdlib.h>
#include <yoc/fota.h>
#include "http.h"
#include <aos/log.h>

#define TAG "http"

http_t *http_init(const char *path)
{
    char *ip, *port_str;
    char *end_point;
    int port;

    http_t *http = aos_zalloc_check(sizeof(http_t));

    http->url = strdup(path);

    http->host = http->url + sizeof("http://") - 1;
    http->port = 80;

    ip = http->url + strlen("http://");

    port_str = strrchr(http->host, ':');

    if (port_str != NULL) {
        *port_str = 0;
        port_str ++;
        port = strtol(port_str, &end_point, 10);
    } else {
        port = 80;
        end_point = strchr(ip, '/');
    }

    if (end_point != NULL) {
        http->path = strdup(end_point);
        LOGD(TAG, "http: path %s", http->path);

        if (http->path == NULL) {
            goto error;
        }
    } else {
        goto error;
    }

    http->buffer = (uint8_t *)malloc(BUFFER_SIZE);
    memset(http->buffer, 0, BUFFER_SIZE);
    if (http->buffer == NULL) {
        goto error;
    }

    *end_point = 0;
    http->host = strdup(ip);
    if (http->host == NULL) {
        goto error;
    }

    LOGD(TAG, "http connect: %s:%d", http->host, port);

    network_init_http(&http->net);
    if (http->net.net_connect(&http->net, http->host, port, SOCK_STREAM) < 0) {
        goto error;
    }

    return http;

error:
    if (http->url) free(http->url);
    if (http->path) free(http->path);
    if (http->host) free(http->host);
    if (http->buffer) free(http->buffer);
    if (http) free(http);

    return NULL;
}

int http_head_sets(http_t *http, const char *key, const char *value)
{
    int len;
    if (http->buffer == NULL) {
        return -1;
    }

    len = snprintf((char*)http->buffer + http->buffer_offset, BUFFER_SIZE - http->buffer_offset,
            "%s: %s\r\n", key, value);
    if (len <= 0) {
        return -1;
    }

    http->buffer_offset += len;

    return 0;
}

int http_head_seti(http_t *http, const char *key, int value)
{
    int len;
    if (http->buffer == NULL) {
        return -1;
    }

    len = snprintf((char*)http->buffer + http->buffer_offset, BUFFER_SIZE - http->buffer_offset,
            "%s: %d\r\n", key, value);
    if (len <= 0) {
        return -1;
    }

    http->buffer_offset += len;

    return 0;
}

int http_post(http_t *http, char *playload, int timeoutms)
{
    char *temp_buff;
    if ((temp_buff = strdup((char *)http->buffer)) == NULL) {
        return -1;
    }

    // temp_buff = strdup((char *)http->buffer);
    memset(http->buffer, 0, BUFFER_SIZE);

    snprintf((char *)http->buffer, BUFFER_SIZE, "POST %s HTTP/1.1\r\n", http->path);

    strcat((char *)http->buffer, temp_buff);

    strcat((char *)http->buffer, "\r\n");

    strcat((char *)http->buffer, playload);

    // LOGD(TAG, "http post sendbuffer: %s", http->buffer);

    free(temp_buff);

    return http->net.net_write(&http->net, http->buffer, strlen((char*)http->buffer), timeoutms);
}

int http_get(http_t *http, int timeoutms)
{
    char *temp_buff;
    if ((temp_buff = strdup((char *)http->buffer)) == NULL) {
        return -1;
    }

    // temp_buff = strdup((char *)http->buffer);
    memset(http->buffer, 0, BUFFER_SIZE);

    snprintf((char *)http->buffer, BUFFER_SIZE, "GET %s HTTP/1.1\r\n", http->path);

    strcat((char *)http->buffer, temp_buff);

    strcat((char *)http->buffer, "\r\n");

    // LOGD(TAG, "http get sendbuffer: %s", http->buffer);

    free(temp_buff);

    return http->net.net_write(&http->net, http->buffer, strlen((char*)http->buffer), timeoutms);
}

static int http_parse_head(http_t *http, char **head)
{
    char *content;
    char *endpoint;
    int content_len;
    char *code_str;
    int code;
    char *end_point;

    *head = strstr((char*)http->buffer, "\r\n\r\n");

    if (*head == NULL) {
        return 0;
    }

    *head += 4;

    if ((content = strstr((char*)http->buffer, "Content-Length: ")) == NULL) {
        code_str = strstr((char *)http->buffer, "HTTP/1.1");

        code = atoi(code_str + strlen("HTTP/1.1"));
        if (code != 206 && code != 200) {
            LOGD(TAG, "http code :%d", code);
            return -1;
        }

        content_len = strtol(*head, &end_point, 16);

        if (end_point == NULL) {
            return 0;
        }
    } else {
        // content_len = atoi(content + strlen("Content-Length: "));
        content_len = strtol(content + strlen("Content-Length: "), &endpoint, 10);

        if (endpoint == NULL || *endpoint != '\r') {
            return -1;
        }
    }

    // LOGD(TAG, "http parse head: %s %d", http->buffer, content_len);
    return content_len;
}

int http_wait_resp(http_t *http, char **head_end, int timeoutms)
{
    int recv_len = 0;
    int len;
    int content_len;

again:
    recv_len = 0;
    *head_end = NULL;
    content_len = 0;
    memset(http->buffer, 0, BUFFER_SIZE);

    while(recv_len < BUFFER_SIZE) {
        len = http->net.net_read(&http->net, http->buffer + recv_len, BUFFER_SIZE - recv_len, timeoutms);

        if (len <= 0) {
            return -1;
        }

        recv_len += len;

        if ((content_len = http_parse_head(http, head_end)) == 0){
            continue;
        }

        if (content_len < 0) {
            return -1;
        } else {
            break;
        }
    }

    if (content_len <= 0 && !(*head_end)) {
        goto again;
    }

    if (BUFFER_SIZE - (*head_end - (char *)http->buffer) < content_len) {
        return -1;
    }

    // LOGD(TAG, "buffer: %.*s", *head_end - (char*)http_ins->buffer, http_ins->buffer);

    while(recv_len < BUFFER_SIZE) {
        if (content_len <= recv_len - (*head_end - (char *)http->buffer)) {
            break;
        }

        len = http->net.net_read(&http->net, http->buffer + recv_len, BUFFER_SIZE - recv_len, timeoutms);

        if (len <= 0) {
            return -1;
        }

        recv_len += len;
    }

    return content_len;
}

char *http_head_get(http_t *http, char *key, int *length)
{
    char *temp_key;
    char *temp_value;
    char *temp_end;

    if ((temp_key = strstr((char *)http->buffer, key)) == NULL) {
        LOGD(TAG, "no key %s", key);
        return NULL;
    }

    if ((temp_value = strstr(temp_key, ":")) == NULL) {
        LOGD(TAG, "no delimiter");
        return NULL;
    }

    if ((temp_end = strstr(temp_value, "\r\n")) == NULL) {
        LOGD(TAG, "no end");
        return NULL;
    }

    *length = (int)(temp_end - temp_value - 2) >= 0 ? (int)(temp_end - temp_value - 2) : 0;// 1 space

    return temp_value + 1;
}

char *http_read_data(http_t *http)
{
    char *buffer;

    buffer = strstr((char *)http->buffer, "\r\n\r\n");

    if (buffer == NULL) {
        return NULL;
    }

    return buffer + 4;
}

int http_deinit(http_t *http)
{
    http->net.net_disconncet(&http->net);

    if (http->url) free(http->url);
    if (http->path) free(http->path);
    if (http->host) free(http->host);
    if (http->buffer) free(http->buffer);
    if (http) free(http);

    return 0;
}

#if 0
    http_t http;
    http_init(&http, url);

    http_head_sets(&http, "Host", host);
    http_head_seti(&http, "Content-Length", 512);
    http_head_seti(&http, "Connection", "keep-alive");


    // "POST %s HTTP/1.1\r\n"
    http_post(&http, url, playload, size);
    http_get(&http, url);

    http_wait_resp(&http);
    http_head_get(&http, "Content-Length", &lenght);
    http_head_get(&http, "Content-Length", &lenght);
    http_read_data(&http, buffer, size);

    http_deinit(&http);



http_get(const char *url, void *buf, size) {
    http_t http;
    http_init(&http, url);

    http_head_sets(&http, "Host", host);
    http_head_seti(&http, "Content-Length", 512);
    http_head_seti(&http, "Connection", "keep-alive");


    http_get(&http, url);

    http_wait_resp(&http);
    http_head_get(&http, "Content-Length", &lenght);
    http_head_get(&http, "Content-Length", &lenght);
    http_read_data(&http, buffer, size);

    http_deinit(&http);

}

http_post(const char *url, char *data, void *buf, size) {
    http_t http;
    http_init(&http);

    http_head_sets(&http, "Host", host);
    http_head_seti(&http, "Content-Length", 512);
    http_head_seti(&http, "Connection", "keep-alive");


    // "POST %s HTTP/1.1\r\n"
    http_post(&http, url, playload, size);

    http_wait_resp(&http);
    http_head_get(&http, "Content-Length", &lenght);
    http_head_get(&http, "Content-Length", &lenght);
    http_read_data(&http, buffer, size);

    http_deinit(&http);

}
#endif
