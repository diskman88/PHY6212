#include <aos/kv.h>
#include <aos/debug.h>

#if defined(CONFIG_KV_SMART)
extern int __kv_setdata(char *key, char *buf, int bufsize);
extern int __kv_getdata(char *key, char *buf, int bufsize);
extern int __kv_del(char *key);
extern int __kv_reset(void);

int aos_kv_set(const char *key, void *value, int len, int sync)
{
    return __kv_setdata((char *)key, value, len);
}

int aos_kv_setstring(const char *key, const char *v)
{
    return __kv_setdata((char *)key, (void *)v, strlen(v));
}

int aos_kv_setfloat(const char *key, float v)
{
    return __kv_setdata((char *)key, (void *)&v, sizeof(v));
}

int aos_kv_setint(const char *key, int v)
{
    return __kv_setdata((char *)key, (void *)&v, sizeof(v));
}

int aos_kv_get(const char *key, void *buffer, int *buffer_len)
{
    if (buffer_len) {
        int ret = __kv_getdata((char *)key, buffer, *buffer_len);

        if (ret > 0) {
            *buffer_len = ret;
        }

        return (ret <= 0) ? -1 : 0;
    }

    return -1;
}

int aos_kv_getstring(const char *key, char *value, int len)
{
    int ret;
    ret = __kv_getdata((char *)key, (char *)value, len - 1);

    if(ret > 0) {
        value[ret < len ? ret : len - 1] = '\0';
    }

    return ret;
}

int aos_kv_getfloat(const char *key, float *value)
{
    int ret = __kv_getdata((char *)key, (char *)value, sizeof(float));
    return (ret == sizeof(float)) ? 0 : -1;
}

int aos_kv_getint(const char *key, int *value)
{
    int ret = __kv_getdata((char *)key, (char *)value, sizeof(int));
    return (ret == sizeof(int)) ? 0 : -1;
}

int aos_kv_del(const char *key)
{
    return __kv_del((char *)key);
}

int aos_kv_reset(void)
{
    return __kv_reset();
}

#endif
