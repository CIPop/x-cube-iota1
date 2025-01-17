// Copyright 2020 IOTA Stiftung
// SPDX-License-Identifier: Apache-2.0

#ifdef CRYPTO_USE_SODIUM
#include "sodium.h"
#elif CRYPTO_USE_MBEDTLS
#include <string.h>
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/md.h"
#include "mbedtls/pkcs5.h"
#include "mbedtls/sha256.h"
#include "mbedtls/sha512.h"
#elif CRYPTO_USE_OPENSSL
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <string.h>
#include <sys/random.h>
#elif CRYPTO_USE_STLIB
#include <stdint.h>
#include <string.h>
#include "ecc/cmox_eddsa.h"
#include "mac/cmox_hmac.h"
#include "hash/cmox_sha256.h"
#include "hash/cmox_sha512.h"
#include "blake2.h"
#else
#error Crypto backend is not defined
#endif

#ifdef CRYPTO_USE_ED25519_DONNA
#include "ed25519.h"
#endif

#ifdef CRYPTO_USE_BLAKE2B_REF
#include "blake2.h"
#endif

#ifdef __ZEPHYR__
#include <random/rand32.h>
#endif

#include <string.h>

#include "crypto/iota_crypto.h"

#ifdef CRYPTO_USE_SODIUM
// store 32 bits in big-endian
static inline void store32_be(uint8_t dst[4], uint32_t w) {
  if (is_little_endian()) {
    dst[3] = (uint8_t)w;
    w >>= 8;
    dst[2] = (uint8_t)w;
    w >>= 8;
    dst[1] = (uint8_t)w;
    w >>= 8;
    dst[0] = (uint8_t)w;
  } else {
    memcpy(dst, &w, sizeof w);
  }
}
#endif /* CRYPTO_USE_SODIUM */

extern struct __RNG_HandleTypeDef hrng;
int iota_rng_raw(void *data, uint8_t *output, size_t len);

void iota_crypto_randombytes(uint8_t *const buf, const size_t len) {
#if defined(CRYPTO_USE_SODIUM) || defined(CRYPTO_USE_STLIB)
  iota_rng_raw(&hrng, buf, len);

// TODO: validate on Mbed OS
// #elif defined(CRYPTO_USE_MBEDTLS) && defined(__MBED__)
//   // TODO use (T)RNG or mbed PSA
//   srand((unsigned int)time(NULL));
//   for (size_t l = 0; l < len; l++) {
//     buf[l] = (uint8_t)rand();
//   }
#elif defined(CRYPTO_USE_MBEDTLS) && defined(__ZEPHYR__)
#if defined(CONFIG_TEST_RANDOM_GENERATOR)
  sys_rand_get(buf, len);
#else
  sys_csrand_get(buf, len);
#endif
#elif defined(CRYPTO_USE_MBEDTLS)
  int ret = 0;
  mbedtls_ctr_drbg_context drbg;
  mbedtls_entropy_context ent;

  mbedtls_ctr_drbg_init(&drbg);
  mbedtls_entropy_init(&ent);

  ret = mbedtls_ctr_drbg_seed(&drbg, mbedtls_entropy_func, &ent, (unsigned char const *)"CTR_DRBG", 8);
  if (ret == 0) {
    mbedtls_ctr_drbg_random(&drbg, buf, len);
  }

  mbedtls_entropy_free(&ent);
  mbedtls_ctr_drbg_free(&drbg);
#elif defined(CRYPTO_USE_OPENSSL)
  RAND_bytes(buf, len);
#else
#error crypto lib is not defined
#endif
}

// get ed25519 public and private key from address
void iota_crypto_keypair(uint8_t const seed[], iota_keypair_t *keypair) {
#if defined(CRYPTO_USE_SODIUM)
  crypto_sign_seed_keypair(keypair->pub, keypair->priv, seed);
#elif defined(CRYPTO_USE_STLIB)
  cmox_ecc_handle_t ctx; /* ECC context */
  uint8_t membuf[548]; /* minimum memory necessary to generate keys */

  /* set the memory buffer and the mathematical engine */
  cmox_ecc_construct(&ctx, CMOX_MATH_FUNCS_SUPERFAST256, membuf, sizeof(membuf));

  /* call the EdDSA key-pair generation routine */
  (void)cmox_eddsa_keyGen(&ctx, CMOX_ECC_ED25519_OPT_HIGHMEM,
                          seed, ED_SEED_BYTES,
                          keypair->priv, NULL,
                          keypair->pub, NULL);

  /* clean & destroy the context */
  cmox_ecc_cleanup(&ctx);
#elif defined(CRYPTO_USE_ED25519_DONNA)
  ed25519_public_key pub;
  ed25519_publickey(seed, pub);
  memcpy(keypair->priv, seed, 32);
  memcpy(keypair->priv + 32, pub, 32);
  memcpy(keypair->pub, pub, 32);
#else
#error ed25519 is not defined
#endif
}

int iota_crypto_sign(uint8_t const priv_key[], uint8_t msg[], size_t msg_len, uint8_t signature[]) {
#if defined(CRYPTO_USE_SODIUM)
  unsigned long long sign_len = ED_SIGNATURE_BYTES;
  return crypto_sign_ed25519_detached(signature, &sign_len, msg, msg_len, priv_key);
#elif defined(CRYPTO_USE_STLIB)
  cmox_ecc_retval_t rv; /* CMOX return value */
  int ret = 1; /* final return value */
  cmox_ecc_handle_t ctx; /* ECC context */
  uint8_t membuf[1388]; /* minimum memory necessary to sign 1kB of data */

  /* set the memory buffer and the mathematical engine */
  cmox_ecc_construct(&ctx, CMOX_MATH_FUNCS_SUPERFAST256, membuf, sizeof(membuf));

  /* call the EdDSA signature generation routine */
  rv = cmox_eddsa_sign(&ctx, CMOX_ECC_ED25519_OPT_HIGHMEM,
                       priv_key, ED_PRIVATE_KEY_BYTES,
                       msg, msg_len,
                       signature, NULL);

  /* clean & destroy the context */
  cmox_ecc_cleanup(&ctx);

  /* set the proper return value */
  if (rv == CMOX_ECC_SUCCESS)
  {
    ret = 0;
  }

  return ret;
#elif defined(CRYPTO_USE_ED25519_DONNA)
  ed25519_sign(msg, msg_len, priv_key, priv_key + 32, signature);
  return 0;
#else
#error ed25519 is not defined
#endif
}

int iota_crypto_hmacsha256(uint8_t const secret_key[], uint8_t msg[], size_t msg_len, uint8_t auth[]) {
#if defined(CRYPTO_USE_SODIUM)
  return crypto_auth_hmacsha256(auth, msg, msg_len, secret_key);
#elif defined(CRYPTO_USE_MBEDTLS)
  int ret = -1;
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (md_info) {
    ret = mbedtls_md_hmac(md_info, secret_key, 32, msg, msg_len, auth);
  }
  return ret;
#elif defined(CRYPTO_USE_STLIB)
  cmox_mac_retval_t rv; /* CMOX return value */
  int ret = 1; /* final return value */

  rv = cmox_mac_compute(CMOX_HMAC_SHA256_ALGO, /* algorithm */
                        msg, msg_len,          /* input */
                        secret_key, 32u,       /* key */
                        NULL, 0u,              /* custom data */
                        auth, 32u, NULL);      /* tag, expected length, output length */
  /* set the proper return value */
  if (rv == CMOX_MAC_SUCCESS)
  {
    ret = 0;
  }

  return ret;
#elif defined(CRYPTO_USE_OPENSSL)
  uint8_t *hash = HMAC(EVP_sha256(), secret_key, 32, (const unsigned char *)msg, msg_len, NULL, NULL);
  memcpy(auth, hash, 32);
  return 0;
#else
#error crypto lib is not defined
#endif
}

int iota_crypto_hmacsha512(uint8_t const secret_key[], uint8_t msg[], size_t msg_len, uint8_t auth[]) {
#if defined(CRYPTO_USE_SODIUM)
  return crypto_auth_hmacsha512(auth, msg, msg_len, secret_key);
#elif defined(CRYPTO_USE_MBEDTLS)
  int ret = -1;
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
  if (md_info) {
    ret = mbedtls_md_hmac(md_info, secret_key, 32, msg, msg_len, auth);
  }
  return ret;
#elif defined(CRYPTO_USE_STLIB)
  cmox_mac_retval_t rv; /* CMOX return value */
  int ret = 1; /* final return value */

  rv = cmox_mac_compute(CMOX_HMAC_SHA512_ALGO, /* algorithm */
                        msg, msg_len,          /* input */
                        secret_key, 32u,       /* key */
                        NULL, 0u,              /* custom data */
                        auth, 64u, NULL);      /* tag, expected length, output length */
  /* set the proper return value */
  if (rv == CMOX_MAC_SUCCESS)
  {
    ret = 0;
  }

  return ret;
#elif defined(CRYPTO_USE_OPENSSL)
  uint8_t *hash = HMAC(EVP_sha512(), secret_key, 32, (const unsigned char *)msg, msg_len, NULL, NULL);
  memcpy(auth, hash, 64);
  return 0;
#else
#error crypto lib is not defined
#endif
}

int iota_blake2b_sum(uint8_t const msg[], size_t msg_len, uint8_t out[], size_t out_len) {
#if defined(CRYPTO_USE_SODIUM) || defined(CRYPTO_USE_STLIB)
  return crypto_generichash_blake2b(out, out_len, msg, msg_len, NULL, 0);
#elif defined(CRYPTO_USE_BLAKE2B_REF)
  return blake2b(out, out_len, msg, msg_len, NULL, 0);
#else
#error blake2b is not defined
#endif
}

int iota_crypto_sha256(uint8_t const msg[], size_t msg_len, uint8_t hash[]) {
#if defined(CRYPTO_USE_SODIUM)
  return crypto_hash_sha256(hash, msg, msg_len);
#elif defined(CRYPTO_USE_STLIB)
  cmox_hash_retval_t rv;

  rv = cmox_hash_compute(CMOX_SHA256_ALGO,
                         msg, msg_len,           /* input and size */
                         hash, CMOX_SHA256_SIZE, /* output digest, and desired size (you may want truncated hash) */
                         NULL);                  /* computed output length, optional */
  if (rv != CMOX_HASH_SUCCESS)
  {
    return -1;
  }
  return 0;
#elif defined(CRYPTO_USE_MBEDTLS)
  mbedtls_sha256(msg, msg_len, hash, 0);
  return 0;
#elif defined(CRYPTO_USE_OPENSSL)
  EVP_MD_CTX *mdctx;
  unsigned int hash_len = CRYPTO_SHA256_HASH_BYTES;
  if ((mdctx = EVP_MD_CTX_new()) != NULL) {
    if (1 == EVP_DigestInit_ex(mdctx, EVP_sha256(), NULL)) {
      if (1 == EVP_DigestUpdate(mdctx, (void const *)msg, msg_len)) {
        if (1 == EVP_DigestFinal_ex(mdctx, hash, &hash_len)) {
          EVP_MD_CTX_free(mdctx);
          return 0;
        }
      }
    }
    EVP_MD_CTX_free(mdctx);
  }
  return -1;
#else
#error crypto lib is not defined
#endif
}

int iota_crypto_sha512(uint8_t const msg[], size_t msg_len, uint8_t hash[]) {
#if defined(CRYPTO_USE_SODIUM)
  return crypto_hash_sha512(hash, msg, msg_len);
#elif defined(CRYPTO_USE_STLIB)
  cmox_hash_retval_t rv;

  rv = cmox_hash_compute(CMOX_SHA512_ALGO,
                         msg, msg_len,           /* input and size */
                         hash, CMOX_SHA512_SIZE, /* output digest, and desired size (you may want truncated hash) */
                         NULL);                  /* computed output length, optional */
  if (rv != CMOX_HASH_SUCCESS)
  {
    return -1;
  }
  return 0;
#elif defined(CRYPTO_USE_MBEDTLS)
  mbedtls_sha512(msg, msg_len, hash, 0);
  return 0;
#elif defined(CRYPTO_USE_OPENSSL)
  EVP_MD_CTX *mdctx;
  unsigned int hash_len = CRYPTO_SHA256_HASH_BYTES;
  if ((mdctx = EVP_MD_CTX_new()) != NULL) {
    if (1 == EVP_DigestInit_ex(mdctx, EVP_sha512(), NULL)) {
      if (1 == EVP_DigestUpdate(mdctx, (void const *)msg, msg_len)) {
        if (1 == EVP_DigestFinal_ex(mdctx, hash, &hash_len)) {
          EVP_MD_CTX_free(mdctx);
          return 0;
        }
      }
    }
    EVP_MD_CTX_free(mdctx);
  }
  return -1;
#else
#error crypto lib is not defined
#endif
}

int iota_crypto_pbkdf2_hmac_sha512(char const pwd[], size_t pwd_len, char const salt[], size_t salt_len,
                                   int32_t iterations, uint8_t dk[], size_t dk_len) {
#if defined(CRYPTO_USE_SODIUM)
  crypto_auth_hmacsha512_state PShctx, hctx;
  size_t i, j, k;
  uint8_t ivec[4];
  uint8_t U[crypto_auth_hmacsha512_BYTES];
  uint8_t T[crypto_auth_hmacsha512_BYTES];
  size_t clen;

  crypto_auth_hmacsha512_init(&PShctx, (uint8_t const *)pwd, pwd_len);
  crypto_auth_hmacsha512_update(&PShctx, (uint8_t const *)salt, salt_len);

  // DK = T1 + T2 + ... + T(dklen/hlen)
  for (i = 0; i * crypto_auth_hmacsha512_BYTES < dk_len; i++) {
    store32_be(ivec, (uint32_t)(i + 1));
    memcpy(&hctx, &PShctx, sizeof(crypto_auth_hmacsha512_state));
    crypto_auth_hmacsha512_update(&hctx, ivec, 4);
    crypto_auth_hmacsha512_final(&hctx, U);
    memcpy(T, U, crypto_auth_hmacsha512_BYTES);

    for (j = 2; j <= iterations; j++) {
      crypto_auth_hmacsha512_init(&hctx, (uint8_t const *)pwd, pwd_len);
      crypto_auth_hmacsha512_update(&hctx, U, crypto_auth_hmacsha512_BYTES);
      crypto_auth_hmacsha512_final(&hctx, U);

      for (k = 0; k < crypto_auth_hmacsha512_BYTES; k++) {
        // XOR
        T[k] ^= U[k];
      }
    }

    clen = dk_len - i * crypto_auth_hmacsha512_BYTES;
    if (clen > crypto_auth_hmacsha512_BYTES) {
      clen = crypto_auth_hmacsha512_BYTES;
    }
    memcpy(&dk[i * crypto_auth_hmacsha512_BYTES], T, clen);
  }

  sodium_memzero((void *)&PShctx, sizeof PShctx);
  return 0;
#elif defined(CRYPTO_USE_STLIB)
  // TODO
  return -1;
#elif defined(CRYPTO_USE_MBEDTLS)
  int ret = -1;
  mbedtls_md_context_t ctx;
  const mbedtls_md_info_t *md_info = mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);

  mbedtls_md_init(&ctx);
  ret = mbedtls_md_setup(&ctx, md_info, 1);
  if (ret == 0) {
    ret = mbedtls_pkcs5_pbkdf2_hmac(&ctx, (unsigned char const *)pwd, pwd_len, (unsigned char const *)salt, salt_len,
                                    iterations, dk_len, dk);
  }
  mbedtls_md_free(&ctx);
  return ret;
#elif defined(CRYPTO_USE_OPENSSL)
  PKCS5_PBKDF2_HMAC(pwd, pwd_len, (unsigned char const *)salt, salt_len, iterations, EVP_sha512(), dk_len, dk);
  return 0;
#else
#error crypto lib is not defined
#endif
}
