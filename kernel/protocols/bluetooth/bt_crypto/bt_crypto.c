/*
 * Copyright (c) 2017 Nordic Semiconductor ASA
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <string.h>
#include <errno.h>

#include <zephyr.h>
#include <misc/byteorder.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>
#include <bluetooth/conn.h>

#include <tinycrypt/constants.h>
#include <tinycrypt/hmac_prng.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/utils.h>
#include <tinycrypt/cmac_mode.h>
#include <tinycrypt/ccm_mode.h>

#define BT_DBG_ENABLED 0
#include "common/log.h"

#include "hci_core.h"
#include "hci_api.h"

#include "bt_crypto.h"

#define __BT_CRYPTO_WEAK__ __attribute__((weak))

static struct tc_hmac_prng_struct prng;

static int prng_reseed(struct tc_hmac_prng_struct *h)
{
    u8_t seed[32];
    s64_t extra;
    int ret, i;

    for (i = 0; i < (sizeof(seed) / 8); i++) {
        hci_api_le_rand(&seed[i * 8]);
    }

    extra = k_uptime_get();

    ret = tc_hmac_prng_reseed(h, seed, sizeof(seed), (u8_t *)&extra,
                              sizeof(extra));

    if (ret == TC_CRYPTO_FAIL) {
        BT_ERR("Failed to re-seed PRNG");
        return -EIO;
    }

    return 0;
}

__BT_CRYPTO_WEAK__ int prng_init(void)
{
    u8_t rand[8];
    int ret;

    hci_api_le_rand(rand);

    ret = tc_hmac_prng_init(&prng, rand, sizeof(rand));

    if (ret == TC_CRYPTO_FAIL) {
        BT_ERR("Failed to initialize PRNG");
        return -EIO;
    }

    /* re-seed is needed after init */
    return prng_reseed(&prng);
}

__BT_CRYPTO_WEAK__ int bt_crypto_rand(void *buf, size_t len)
{
    int ret;

    ret = tc_hmac_prng_generate(buf, len, &prng);

    if (ret == TC_HMAC_PRNG_RESEED_REQ) {
        ret = prng_reseed(&prng);

        if (ret) {
            return ret;
        }

        ret = tc_hmac_prng_generate(buf, len, &prng);
    }

    if (ret == TC_CRYPTO_SUCCESS) {
        return 0;
    }

    return -EIO;
}

__BT_CRYPTO_WEAK__ int bt_crypto_encrypt_le(const u8_t key[16], const u8_t plaintext[16],
        u8_t enc_data[16])
{
    struct tc_aes_key_sched_struct s;
    u8_t tmp[16];

    BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

    sys_memcpy_swap(tmp, key, 16);

    if (tc_aes128_set_encrypt_key(&s, tmp) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    sys_memcpy_swap(tmp, plaintext, 16);

    if (tc_aes_encrypt(enc_data, tmp, &s) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    sys_mem_swap(enc_data, 16);

    BT_DBG("enc_data %s", bt_hex(enc_data, 16));

    return 0;
}

__BT_CRYPTO_WEAK__ int bt_crypto_encrypt_be(const u8_t key[16], const u8_t plaintext[16],
        u8_t enc_data[16])
{
    struct tc_aes_key_sched_struct s;

    BT_DBG("key %s plaintext %s", bt_hex(key, 16), bt_hex(plaintext, 16));

    if (tc_aes128_set_encrypt_key(&s, key) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    if (tc_aes_encrypt(enc_data, plaintext, &s) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    BT_DBG("enc_data %s", bt_hex(enc_data, 16));

    return 0;
}

__BT_CRYPTO_WEAK__ int bt_crypto_decrypt_be(const u8_t key[16], const u8_t enc_data[16],
		  u8_t dec_data[16])
{
    struct tc_aes_key_sched_struct s;

    if (tc_aes128_set_decrypt_key(&s, key) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    if (tc_aes_decrypt(dec_data, enc_data, &s) == TC_CRYPTO_FAIL) {
        return -EINVAL;
    }

    BT_DBG("dec_data %s", bt_hex(dec_data, 16));

    return 0;
}


__BT_CRYPTO_WEAK__ int bt_crypto_aes_cmac(const u8_t key[16], struct bt_crypto_sg *sg,
        size_t sg_len, u8_t mac[16])
{
    struct tc_aes_key_sched_struct sched;
    struct tc_cmac_struct state;

    if (tc_cmac_setup(&state, key, &sched) == TC_CRYPTO_FAIL) {
        return -EIO;
    }

    for (; sg_len; sg_len--, sg++) {
        if (tc_cmac_update(&state, sg->data,
                           sg->len) == TC_CRYPTO_FAIL) {
            return -EIO;
        }
    }

    if (tc_cmac_final(mac, &state) == TC_CRYPTO_FAIL) {
        return -EIO;
    }

    return 0;
}
