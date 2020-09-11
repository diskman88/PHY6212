/* log.c - logging helpers */

/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Helper for printk parameters to convert from binary to hex.
 * We declare multiple buffers so the helper can be used multiple times
 * in a single printk call.
 */

#include <stddef.h>
#include <zephyr/types.h>
#include <zephyr.h>
#include <misc/util.h>
#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

const char *bt_hex(const void *buf, size_t len)
{
	static const char hex[] = "0123456789abcdef";
	static char hexbufs[4][129] __attribute__((section("noretention_mem_area0")));
	static u8_t curbuf;
	const u8_t *b = buf;
	unsigned int mask;
	char *str;
	int i;

	mask = irq_lock();
	str = hexbufs[curbuf++];
	curbuf %= ARRAY_SIZE(hexbufs);
	irq_unlock(mask);

	len = min(len, (sizeof(hexbufs[0]) - 1) / 2);

	for (i = 0; i < len; i++) {
		str[i * 2]     = hex[b[i] >> 4];
		str[i * 2 + 1] = hex[b[i] & 0xf];
	}

	str[i * 2] = '\0';

	return str;
}

u8_t stringtohex(char *str, u8_t *out, u8_t count)
{
    u8_t i = 0, j = 0;
    u8_t n = 0;

    memset(out , 0, count);
    if(strlen(str) > count<<1)
    {
        //BT_PRT("Lancelot string size error, %d\r\n", strlen(str));
        return 0;
    }

    while(i < count)
    {
        while(j < 2)
        {
            n = i*2+j;
            if(str[n] >= 'a' && str[n] <= 'f')
            {
                out[i] = out[i]<<4 | (str[n] - 'a' + 10);
            }
            else if(str[n] >= 'A' && str[n] <= 'F')
            {
                out[i] = out[i]<<4 | (str[n] - 'A' + 10);
            }
            else if(str[n] >= '0' && str[n] <= '9')
            {
                out[i] = out[i]<<4 | (str[n] - '0');
            }
            else
            {
                //BT_PRT("Lancelot string error, %c\r\n", str[n]);
                return 0;
            }
            j++;
        }
        //BT_PRT(">>%02x %d %d\r\n", out[i], i ,j);
        j = 0;
        i++;
    }
    return count;
}

#if defined(CONFIG_BT_DEBUG)
const char *bt_addr_str(const bt_addr_t *addr)
{
	static char bufs[2][18];
	static u8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_to_str(addr, str, sizeof(bufs[cur]));

	return str;
}

const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
	static char bufs[2][30];
	static u8_t cur;
	char *str;

	str = bufs[cur++];
	cur %= ARRAY_SIZE(bufs);
	bt_addr_le_to_str(addr, str, sizeof(bufs[cur]));

	return str;
}
#else
const char *bt_addr_str(const bt_addr_t *addr)
{
    return "";
}

const char *bt_addr_le_str(const bt_addr_le_t *addr)
{
    return "";
}
#endif /* CONFIG_BT_DEBUG */

