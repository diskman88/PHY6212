/*
 * Copyright (C) 2017 C-SKY Microsystems Co., All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

struct bt_crypto_sg {
	const void *data;
	size_t len;
};

int bt_crypto_aes_cmac(const uint8_t key[16], struct bt_crypto_sg *sg,
		     size_t sg_len, uint8_t mac[16]);

int bt_crypto_rand(void *buf, size_t len);

int bt_crypto_encrypt_le(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16]);

int bt_crypto_encrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16]);

int bt_crypto_decrypt_be(const uint8_t key[16], const uint8_t plaintext[16],
		  uint8_t enc_data[16]);

