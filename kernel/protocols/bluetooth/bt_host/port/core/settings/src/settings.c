/*
 * Copyright (c) 2018 Nordic Semiconductor ASA
 * Copyright (c) 2015 Runtime Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#include <errno.h>

//#include "base64.h"

#include "settings/settings.h"
#include "settings_priv.h"
#include <zephyr/types.h>

/* mbedtls-base64 lib encodes data to null-terminated string */
#define BASE64_ENCODE_SIZE(in_size) ((((((in_size) - 1) / 3) * 4) + 4) + 1)

sys_slist_t settings_handlers;

static u8_t settings_cmd_inited;

void settings_store_init(void);
static void s64_to_dec(char *ptr, int buf_len, s64_t value, int base);
static s64_t dec_to_s64(char *p_str, char **e_ptr);


static const unsigned char base64_enc_map[64] =
{
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
    'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd',
    'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
    'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', '+', '/'
};

static const unsigned char base64_dec_map[128] =
{
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127, 127, 127, 127, 127, 127, 127, 127,
    127, 127, 127,  62, 127, 127, 127,  63,  52,  53,
     54,  55,  56,  57,  58,  59,  60,  61, 127, 127,
    127,  64, 127, 127, 127,   0,   1,   2,   3,   4,
      5,   6,   7,   8,   9,  10,  11,  12,  13,  14,
     15,  16,  17,  18,  19,  20,  21,  22,  23,  24,
     25, 127, 127, 127, 127, 127, 127,  26,  27,  28,
     29,  30,  31,  32,  33,  34,  35,  36,  37,  38,
     39,  40,  41,  42,  43,  44,  45,  46,  47,  48,
     49,  50,  51, 127, 127, 127, 127, 127
};

#define BASE64_SIZE_T_MAX   ( (size_t) -1 ) /* SIZE_T_MAX is not standard */

/*
 * Encode a buffer into base64 format
 */
int mbedtls_base64_encode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen )
{
    size_t i, n;
    int C1, C2, C3;
    unsigned char *p;

    if( slen == 0 )
    {
        *olen = 0;
        return( 0 );
    }

    n = slen / 3 + ( slen % 3 != 0 );

    if( n > ( BASE64_SIZE_T_MAX - 1 ) / 4 )
    {
        *olen = BASE64_SIZE_T_MAX;
        return -1;
    }

    n *= 4;

    if( ( dlen < n + 1 ) || ( NULL == dst ) )
    {
        *olen = n + 1;
        return( -1 );
    }

    n = ( slen / 3 ) * 3;

    for( i = 0, p = dst; i < n; i += 3 )
    {
        C1 = *src++;
        C2 = *src++;
        C3 = *src++;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 &  3) << 4) + (C2 >> 4)) & 0x3F];
        *p++ = base64_enc_map[(((C2 & 15) << 2) + (C3 >> 6)) & 0x3F];
        *p++ = base64_enc_map[C3 & 0x3F];
    }

    if( i < slen )
    {
        C1 = *src++;
        C2 = ( ( i + 1 ) < slen ) ? *src++ : 0;

        *p++ = base64_enc_map[(C1 >> 2) & 0x3F];
        *p++ = base64_enc_map[(((C1 & 3) << 4) + (C2 >> 4)) & 0x3F];

        if( ( i + 1 ) < slen )
             *p++ = base64_enc_map[((C2 & 15) << 2) & 0x3F];
        else *p++ = '=';

        *p++ = '=';
    }

    *olen = p - dst;
    *p = 0;

    return( 0 );
}

/*
 * Decode a base64-formatted buffer
 */
int mbedtls_base64_decode( unsigned char *dst, size_t dlen, size_t *olen,
                   const unsigned char *src, size_t slen )
{
    size_t i, n;
    uint32_t j, x;
    unsigned char *p;

    /* First pass: check for validity and get output length */
    for( i = n = j = 0; i < slen; i++ )
    {
        /* Skip spaces before checking for EOL */
        x = 0;
        while( i < slen && src[i] == ' ' )
        {
            ++i;
            ++x;
        }

        /* Spaces at end of buffer are OK */
        if( i == slen )
            break;

        if( ( slen - i ) >= 2 &&
            src[i] == '\r' && src[i + 1] == '\n' )
            continue;

        if( src[i] == '\n' )
            continue;

        /* Space inside a line is an error */
        if( x != 0 )
            return( -1 );

        if( src[i] == '=' && ++j > 2 )
            return( -1 );

        if( src[i] > 127 || base64_dec_map[src[i]] == 127 )
            return( -1 );

        if( base64_dec_map[src[i]] < 64 && j != 0 )
            return( -1 );

        n++;
    }

    if( n == 0 )
    {
        *olen = 0;
        return( 0 );
    }

    /* The following expression is to calculate the following formula without
     * risk of integer overflow in n:
     *     n = ( ( n * 6 ) + 7 ) >> 3;
     */
    n = ( 6 * ( n >> 3 ) ) + ( ( 6 * ( n & 0x7 ) + 7 ) >> 3 );
    n -= j;

    if( dst == NULL || dlen < n )
    {
        *olen = n;
        return( -1 );
    }

   for( j = 3, n = x = 0, p = dst; i > 0; i--, src++ )
   {
        if( *src == '\r' || *src == '\n' || *src == ' ' )
            continue;

        j -= ( base64_dec_map[*src] == 64 );
        x  = ( x << 6 ) | ( base64_dec_map[*src] & 0x3F );

        if( ++n == 4 )
        {
            n = 0;
            if( j > 0 ) *p++ = (unsigned char)( x >> 16 );
            if( j > 1 ) *p++ = (unsigned char)( x >>  8 );
            if( j > 2 ) *p++ = (unsigned char)( x       );
        }
    }

    *olen = p - dst;

    return( 0 );
}

void settings_init(void)
{
	if (!settings_cmd_inited) {
		sys_slist_init(&settings_handlers);
		settings_store_init();

		settings_cmd_inited = 1;
	}
}

int settings_register(struct settings_handler *handler)
{
	sys_slist_prepend(&settings_handlers, &handler->node);
	return 0;
}

/*
 * Find settings_handler based on name.
 */
struct settings_handler *settings_handler_lookup(char *name)
{
	struct settings_handler *ch;

	SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
		if (!strcmp(name, ch->name)) {
			return ch;
		}
	}
	return NULL;
}

/*
 * Separate string into argv array.
 */
int settings_parse_name(char *name, int *name_argc, char *name_argv[])
{
	int i = 0;

	while (name) {
		name_argv[i++] = name;

		while (1) {
			if (*name == '\0') {
				name = NULL;
				break;
			}

			if (*name == *SETTINGS_NAME_SEPARATOR) {
				*name = '\0';
				name++;
				break;
			}
			name++;
		}
	}

	*name_argc = i;

	return 0;
}

static struct settings_handler *settings_parse_and_lookup(char *name,
							  int *name_argc,
							  char *name_argv[])
{
	int rc;

	rc = settings_parse_name(name, name_argc, name_argv);
	if (rc) {
		return NULL;
	}
	return settings_handler_lookup(name_argv[0]);
}

int settings_value_from_str(char *val_str, enum settings_type type, void *vp,
			    int maxlen)
{
	s32_t val;
	s64_t val64;
	char *eptr;

	if (!val_str) {
		goto err;
	}
	switch (type) {
	case SETTINGS_INT8:
	case SETTINGS_INT16:
	case SETTINGS_INT32:
	case SETTINGS_BOOL:
		val = strtol(val_str, &eptr, 0);
		if (*eptr != '\0') {
			goto err;
		}
		if (type == SETTINGS_BOOL) {
			if (val < 0 || val > 1) {
				goto err;
			}
			*(bool *)vp = val;
		} else if (type == SETTINGS_INT8) {
			if (val < INT8_MIN || val > UINT8_MAX) {
				goto err;
			}
			*(int8_t *)vp = val;
		} else if (type == SETTINGS_INT16) {
			if (val < INT16_MIN || val > UINT16_MAX) {
				goto err;
			}
			*(int16_t *)vp = val;
		} else if (type == SETTINGS_INT32) {
			*(s32_t *)vp = val;
		}
		break;
	case SETTINGS_INT64:
		val64 = dec_to_s64(val_str, &eptr);
		if (*eptr != '\0') {
			goto err;
		}
		*(s64_t *)vp = val64;
		break;
	case SETTINGS_STRING:
		val = strlen(val_str);
		if (val + 1 > maxlen) {
			goto err;
		}
		strcpy(vp, val_str);
		break;
	default:
		goto err;
	}
	return 0;
err:
	return -EINVAL;
}

int settings_bytes_from_str(char *val_str, void *vp, int *len)
{
	int err;
	size_t rc;

    err = mbedtls_base64_decode(vp, *len, &rc, (const unsigned char *)val_str, strlen(val_str));

	if (err) {
		return -1;
	}

	*len = rc;
	return 0;
}

char *settings_str_from_value(enum settings_type type, void *vp, char *buf,
			      int buf_len)
{
	s32_t val;

	if (type == SETTINGS_STRING) {
		return vp;
	}
	switch (type) {
	case SETTINGS_INT8:
	case SETTINGS_INT16:
	case SETTINGS_INT32:
	case SETTINGS_BOOL:
		if (type == SETTINGS_BOOL) {
			val = *(bool *)vp;
		} else if (type == SETTINGS_INT8) {
			val = *(int8_t *)vp;
		} else if (type == SETTINGS_INT16) {
			val = *(int16_t *)vp;
		} else {
			val = *(s32_t *)vp;
		}
		snprintf(buf, buf_len, "%ld", (long)val);
		return buf;
	case SETTINGS_INT64:
		s64_to_dec(buf, buf_len, *(s64_t *)vp, 10);
		return buf;
	default:
		return NULL;
	}
}

static void u64_to_dec(char *ptr, int buf_len, u64_t value, int base)
{
	u64_t t = 0, res = 0;
	u64_t tmp = value;
	int count = 0;

	if (ptr == NULL) {
		return;
	}

	if (tmp == 0) {
		count++;
	}

	while (tmp > 0) {
		tmp = tmp/base;
		count++;
	}

	ptr += count;

	*ptr = '\0';

	do {
		res = value - base * (t = value / base);
		if (res < 10) {
			*--ptr = '0' + res;
		} else if ((res >= 10) && (res < 16)) {
			*--ptr = 'A' - 10 + res;
		}
		value = t;
	} while (value != 0);
}

static void s64_to_dec(char *ptr, int buf_len, s64_t value, int base)
{
	u64_t val64;

	if (ptr == NULL || buf_len < 1) {
		return;
	}

	if (value < 0) {
		*ptr = '-';
		ptr++;
		buf_len--;
		val64 = value * (-1);
	} else {
		val64 = value;
	}

	u64_to_dec(ptr, buf_len, val64, base);
}

static s64_t dec_to_s64(char *p_str, char **e_ptr)
{
	u64_t val = 0, prev_val = 0;
	bool neg = false;
	int digit;

	if (*p_str == '-') {
		neg = true;
		p_str++;
	} else if (*p_str == '+') {
		p_str++;
	}

	while (1) {
		if (*p_str >= '0' && *p_str <= '9') {
			digit = *p_str - '0';
		} else {
			break;
		}

		val *= 10;
		val += digit;

		/* this is only a fuse */
		if (val < prev_val) {
			break;
		}

		prev_val = val;
		p_str++;
	}

	if (e_ptr != 0)
		*e_ptr = p_str;

	if (neg) {
		return -val;
	} else {
		return val;
	}
}

char *settings_str_from_bytes(void *vp, int vp_len, char *buf, int buf_len)
{
	if (BASE64_ENCODE_SIZE(vp_len) > buf_len) {
		return NULL;
	}

	size_t enc_len;

    mbedtls_base64_encode((unsigned char *)buf, buf_len, &enc_len, vp, vp_len);

	return buf;
}

int settings_set_value(char *name, char *val_str)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return -EINVAL;
	}

	return ch->h_set(name_argc - 1, &name_argv[1], val_str);
}

/*
 * Get value in printable string form. If value is not string, the value
 * will be filled in *buf.
 * Return value will be pointer to beginning of that buffer,
 * except for string it will pointer to beginning of string.
 */
char *settings_get_value(char *name, char *buf, int buf_len)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;

	ch = settings_parse_and_lookup(name, &name_argc, name_argv);
	if (!ch) {
		return NULL;
	}

	if (!ch->h_get) {
		return NULL;
	}
	return ch->h_get(name_argc - 1, &name_argv[1], buf, buf_len);
}

int settings_commit(char *name)
{
	int name_argc;
	char *name_argv[SETTINGS_MAX_DIR_DEPTH];
	struct settings_handler *ch;
	int rc;
	int rc2;

	if (name) {
		ch = settings_parse_and_lookup(name, &name_argc, name_argv);
		if (!ch) {
			return -EINVAL;
		}
		if (ch->h_commit) {
			return ch->h_commit();
		} else {
			return 0;
		}
	} else {
		rc = 0;
		SYS_SLIST_FOR_EACH_CONTAINER(&settings_handlers, ch, node) {
			if (ch->h_commit) {
				rc2 = ch->h_commit();
				if (!rc) {
					rc = rc2;
				}
			}
		}
		return rc;
	}
}
