#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <btc/options.h>
#include <btc/bip39.h>
#include <btc/hmac.h>
#include <btc/pbkdf2.h>
#include <btc/rand.h>
#include <btc/sha2mnemonic.h>
#include <btc/bip39_english.h>
#include <btc/memzero.h>
//lib btc
#include <btc/ecc_key.h>
#include <btc/random.h>


#if USE_BIP39_CACHE

static int bip39_cache_index = 0;

static CONFIDENTIAL struct {
	bool set;
	char mnemonic[256];
	char passphrase[64];
	uint8_t seed[512 / 8];
} bip39_cache[BIP39_CACHE_SIZE];

#endif

// int main() {
// 	uint8_t seed[64];
// 	const char *mnemonic_phrase = mnemonic_generate(128);

// 	mnemonic_to_seed(mnemonic_phrase, "TEST", seed, 0);

// 	printf("Mnemonic Phrase: %s\n", bip39_cache[0].mnemonic);
// 	printf("Seed: ");
//    	for(int i = 0; i < 32; i++){
// 		printf("%x", seed[i]);
// 	}
// 	printf("\n");
//     return 0;
// }

const char *mnemonic_generate(int strength)
{
	if (strength % 32 || strength < 128 || strength > 256) {
		return 0;
	}
	uint8_t data[32];
	// random_buffer(data, 32);
	btc_random_bytes(data, BTC_ECKEY_PKEY_LENGTH, 0);
	// for(int i = 0; i < 32; i++){
	// 	printf("%x\n", data[i]);
	// }
	const char *r = mnemonic_from_data(data, strength / 8);
	memzero(data, sizeof(data));
	return r;
}

char *mnemonic_from_data(const uint8_t *data, int len)
{
	if (len % 4 || len < 16 || len > 32) {
		return 0;
	}

	uint8_t bits[32 + 1];

	trezor_sha256_Raw(data, len, bits);
	// checksum
	bits[len] = bits[0];
	// data
	memcpy(bits, data, len);

	int mlen = len * 3 / 4;
	static CONFIDENTIAL char mnemo[24 * 10];

	int i, j, idx;
	char *p = mnemo;
	for (i = 0; i < mlen; i++) {
		idx = 0;
		for (j = 0; j < 11; j++) {
			idx <<= 1;
			idx += (bits[(i * 11 + j) / 8] & (1 << (7 - ((i * 11 + j) % 8)))) > 0;
		}
		strcpy(p, wordlist[idx]);
		p += strlen(wordlist[idx]);
		*p = (i < mlen - 1) ? ' ' : 0;
		p++;
	}
	memzero(bits, sizeof(bits));

	return mnemo;
}

// passphrase must be at most 256 characters or code may crash
void mnemonic_to_seed(const char *mnemonic, const char *passphrase, uint8_t seed[512 / 8], void (*progress_callback)(uint32_t current, uint32_t total))
{
	int passphraselen = strlen(passphrase);
#if USE_BIP39_CACHE
	int mnemoniclen = strlen(mnemonic);
	// check cache
	if (mnemoniclen < 256 && passphraselen < 64) {
		for (int i = 0; i < BIP39_CACHE_SIZE; i++) {
			if (!bip39_cache[i].set) continue;
			if (strcmp(bip39_cache[i].mnemonic, mnemonic) != 0) continue;
			if (strcmp(bip39_cache[i].passphrase, passphrase) != 0) continue;
			// found the correct entry
			memcpy(seed, bip39_cache[i].seed, 512 / 8);
			return;
		}
	}
#endif
	uint8_t salt[8 + 256];
	memcpy(salt, "mnemonic", 8);
	memcpy(salt + 8, passphrase, passphraselen);
	static CONFIDENTIAL PBKDF2_HMAC_SHA512_CTX pctx;
	pbkdf2_hmac_sha512_Init(&pctx, (const uint8_t *)mnemonic, strlen(mnemonic), salt, passphraselen + 8, 1);
	if (progress_callback) {
		progress_callback(0, BIP39_PBKDF2_ROUNDS);
	}
	for (int i = 0; i < 16; i++) {
		pbkdf2_hmac_sha512_Update(&pctx, BIP39_PBKDF2_ROUNDS / 16);
		if (progress_callback) {
			progress_callback((i + 1) * BIP39_PBKDF2_ROUNDS / 16, BIP39_PBKDF2_ROUNDS);
		}
	}
	pbkdf2_hmac_sha512_Final(&pctx, seed);
	memzero(salt, sizeof(salt));
#if USE_BIP39_CACHE
	// store to cache
	if (mnemoniclen < 256 && passphraselen < 64) {
		bip39_cache[bip39_cache_index].set = true;
		strcpy(bip39_cache[bip39_cache_index].mnemonic, mnemonic);
		strcpy(bip39_cache[bip39_cache_index].passphrase, passphrase);
		memcpy(bip39_cache[bip39_cache_index].seed, seed, 512 / 8);
		bip39_cache_index = (bip39_cache_index + 1) % BIP39_CACHE_SIZE;
	}
#endif
}

int mnemonic_to_entropy(const char *mnemonic, uint8_t* entropy)
{
	if (!mnemonic) {
		return 0;
	}

	uint32_t i = 0, n = 0;

	while (mnemonic[i]) {
		if (mnemonic[i] == ' ') {
			n++;
		}
		i++;
	}
	n++;

	// check number of words
	if (n != 12 && n != 18 && n != 24) {
		return 0;
	}

	char current_word[10];
	uint32_t j, k, ki, bi = 0;
	uint8_t bits[32 + 1];

	memzero(bits, sizeof(bits));
	i = 0;
	while (mnemonic[i]) {
		j = 0;
		while (mnemonic[i] != ' ' && mnemonic[i] != 0) {
			if (j >= sizeof(current_word) - 1) {
				return 0;
			}
			current_word[j] = mnemonic[i];
			i++; j++;
		}
		current_word[j] = 0;
		if (mnemonic[i] != 0) {
			i++;
		}
		k = 0;
		for (;;) {
			if (!wordlist[k]) { // word not found
				return 0;
			}
			if (strcmp(current_word, wordlist[k]) == 0) { // word found on index k
				for (ki = 0; ki < 11; ki++) {
					if (k & (1 << (10 - ki))) {
						bits[bi / 8] |= 1 << (7 - (bi % 8));
					}
					bi++;
				}
				break;
			}
			k++;
		}
	}
	if (bi != n * 11) {
		return 0;
	}
	memcpy(entropy, bits, sizeof(bits));
	return n * 11;
}
