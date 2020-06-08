#include <yoc_config.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <yoc/nvram.h>
#include <yoc/sysinfo.h>

#include <aos/log.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include "drv_tee.h"

static const char* TAG = "COP";

#if 0
#define  COP_ACTIVE_URL_OPEN    "open.c-sky.com"
#define  COP_ACTIVE_URL_CID     "cid.c-sky.com"
#endif

#ifndef  AUTH_SERVER_PORT
#define  AUTH_SERVER_PORT    80
#endif

#define  AUTH_ACTIVE_PATH   "/api/device/activeOneNetDevice"

#define  HTTP_PACKET_MAX     1024
#define  TCP_TIMEOUT         2
#define  PACKET_MAGIC_END   "\r\n0\r\n\r\n"

typedef struct {
    uint8_t* buf;
    uint32_t len;
}storage;

typedef struct {
    int      my_socket;
    storage* send_buf;
    storage* recv_buf;
} auth_network;

static void network_init(auth_network* n, storage* send, storage* recv)
{
    n->my_socket = 0;
    n->send_buf  = send;
    n->recv_buf  = recv;
}

static int network_connect(auth_network *n, const char *addr, int port)
{
    int type = SOCK_STREAM;
    struct sockaddr_in address;
    int rc = -1;
    sa_family_t family = AF_INET;
    struct addrinfo *result = NULL;
    struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
    struct timeval interval = {TCP_TIMEOUT, 0};

    rc = getaddrinfo(addr, NULL, &hints, &result);
    if (rc < 0) {
        return -1;
    }

    struct addrinfo *res = result;

    /* prefer ip4 addresses */
    while (res) {
        if (res->ai_family == AF_INET) {
            result = res;
            break;
        }

        res = res->ai_next;
    }

    if (res == NULL) {
        freeaddrinfo(result);
        return -1;
    }

    address.sin_port = htons(port);
    address.sin_family = family = AF_INET;
    address.sin_addr = ((struct sockaddr_in *)(result->ai_addr))->sin_addr;

    freeaddrinfo(result);

    n->my_socket = socket(family, type, 0);

    if (n->my_socket < 0) {
        return -1;
    }

    if (connect(n->my_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        goto fail;
    }

    if (setsockopt(n->my_socket, SOL_SOCKET, SO_SNDTIMEO, (char *)&interval,
               sizeof(struct timeval))) {
        goto fail;
    }

    if (setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,
               sizeof(struct timeval))) {
        goto fail;
    }

    return 0;
fail:
    closesocket(n->my_socket);
    return -1;
}

static void network_disconnect(auth_network *n)
{
    close(n->my_socket);
    n->my_socket = 0;
}

static int network_recv(auth_network* n)
{
    struct timeval interval = {TCP_TIMEOUT, 0};

    if (setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval,
               sizeof(struct timeval))) {
        return -2;
    }

    int bytes = 0;

    while (bytes < n->recv_buf->len - 1) {
        int rc = recv(n->my_socket, n->recv_buf->buf + bytes,
                      (size_t)(n->recv_buf->len - bytes - 1), 0);

        if (rc <= 0) {
            break;
        } else {
            bytes += rc;
        }

        /* Check packet end */
        if (memcmp(n->recv_buf->buf + bytes - 7, PACKET_MAGIC_END,
                               sizeof(PACKET_MAGIC_END) - 1) == 0) {
            /* set the string end */
            *(n->recv_buf->buf + bytes + 1) = 0;
            LOGD(TAG, "packet fit");
            return bytes;
        }
    }

    return -1;
}

static int network_send(auth_network *n, int len)
{
    int rc = send(n->my_socket, n->send_buf->buf, (size_t)len, 0);

    return rc;
}

static int http_fill_post_package(
                 char       *pack,
                 uint32_t    size,
                 const char *path,
                 const char *url,
                 const char *content)
{
    sprintf(pack, "POST %s HTTP/1.1\r\n", path);
    sprintf((pack + strlen(pack)), "Host: %s\r\n", url);
    sprintf((pack + strlen(pack)), "Content-Length: %d\r\n", strlen(content));
    strcat(pack, "Content-Type: application/json\r\n");
    strcat(pack, "\r\n");

    if (strlen(pack) + strlen(content) > size) {
        LOGE(TAG, "package size too short");
        return -1;
    }
    strcat(pack, content);

    return 0;
}

#if 0
static int json_parse_string(
        char* package,
        char* name,
        int   name_len,
        char* value,
        int   value_len)
{
    char* json;
    char* pos;
    int   pos_len;

    if ((json = strstr(package, name)) == NULL) {
        LOGE(TAG, "http return code error no %s", name);
        return -1;
    }
    json = strchr(json + name_len, '"');
    if ((pos_len = strlen(json) - strlen(strchr(json + 1, '"')))
                 > value_len || pos_len <= 0) {
        LOGE(TAG, "%s too loog, len: %d", name, pos_len);
        return -1;
    }
    if ((pos = strchr(json, '"') + 1) == NULL) {
        LOGE(TAG, "http return code error no %s", name);
        return -1;
    }
    snprintf(value, pos_len, pos);

    return 0;
}
#endif

static int json_parse_int(
        char*     package,
        char*     name,
        int       name_len,
        uint32_t* value)
{
    char* json;

    if ((json = strstr(package, name)) == NULL){
        return -1;
    }
    json   = strchr(json, ':');
    /*if there has no ""*/
    *value = atoi(json + 1);
    /*if there has ""*/
    if (*value == 0){
        *value = atoi(strchr(json + 1, '"') + 1);
    }

    return 0;
}

static int json_code_check(char* package)
{
    int rc;

    if ((json_parse_int(package, "\"code\"", strlen("\"code\""), (uint32_t*)&rc)) != 0) {
        return -1;
    }
    if (rc != 0){
        return -1;
    }

    return 0;
}

static int http_active(auth_network* n, const char* cid, const char* imei, const char* imsi, const char* url)
{
    char content[120] = {0};
    int  rc;

    snprintf(content,  sizeof(content), "{\"cid\":\"%s\",\"imei\":\"%s\",\"imsi\":\"%s\"}", cid, imei, imsi);

    if ((rc = http_fill_post_package((char*)n->send_buf->buf,
        n->send_buf->len, AUTH_ACTIVE_PATH, url, content)) != 0) {
        return rc;
    }

    LOGD(TAG, "send: \r\n %s", n->send_buf->buf);

    if ((rc = network_send(n, strlen((char*)n->send_buf->buf))) < 0) {
        return rc;
    }

    if ((rc = network_recv(n)) <= 0) {
        return rc;
    }

    LOGD(TAG, "revc: \r\n %s", n->recv_buf->buf);

    if ((rc =  json_code_check((char*)n->recv_buf->buf)) != 0) {
        return rc;
    }

    return 0;
}

static int cop_active(const char* cid, const char* imei, const char* imsi, const char* url) {
    int rc;
    auth_network net;
    storage send_buf, recv_buf;

    if (cid == NULL || imei == NULL || imsi == NULL || url == NULL) {
        return -1;
    }

    if ((send_buf.buf = malloc(HTTP_PACKET_MAX)) == NULL) {
        return -1;
    }

    if ((recv_buf.buf = malloc(HTTP_PACKET_MAX)) == NULL) {
        free(send_buf.buf);
        return -1;
    }

    send_buf.len = HTTP_PACKET_MAX;
    recv_buf.len = HTTP_PACKET_MAX;
    network_init(&net, &send_buf, &recv_buf);

    if ((rc = network_connect(&net,
              url, AUTH_SERVER_PORT)) != 0) {
        free(send_buf.buf);
        free(recv_buf.buf);
        return rc;
    }

    if ((rc = http_active(&net, cid, imei, imsi, url)) != 0) {
        network_disconnect(&net);
        free(send_buf.buf);
        free(recv_buf.buf);
        return rc;
    }

    network_disconnect(&net);
    free(send_buf.buf);
    free(recv_buf.buf);

    return 0;
}

int cop_register(const char* url)
{
    const char *imei = aos_get_imei();
    const char *imsi = aos_get_imsi();
    const char *cid = aos_get_device_id();

    if (imei == NULL || imsi == NULL || cid == NULL) {
        return -1;
    }

    if (cop_active(cid, imei, imsi, url)) {
        return -2;
    }

    return 0;
}
