/* Copyright © 2007 Carl Worth
 * Copyright © 2009 Jeremy Huddleston, Julien Cristau, and Matthieu Herrb
 * Copyright © 2009-2010 Mikhail Gusarov
 * Copyright © 2012 Yaakov Selkowitz and Keith Packard
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifdef HAVE_DIX_CONFIG_H
#include <dix-config.h>
#endif

#include "os.h"
#include "xsha1.h"

#if defined(HAVE_SHA1_IN_LIBMD)  /* Use libmd for SHA1 */ \
	|| defined(HAVE_SHA1_IN_LIBC)   /* Use libc for SHA1 */

#if defined(__DragonFly__) || defined(__FreeBSD__)
#include <sha.h>
#define	SHA1End		SHA1_End
#define	SHA1File	SHA1_File
#define	SHA1Final	SHA1_Final
#define	SHA1Init	SHA1_Init
#define	SHA1Update	SHA1_Update
#else
#include <sha1.h>
#endif

void *
x_sha1_init(void)
{
    SHA1_CTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    SHA1Init(ctx);
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    SHA1_CTX *sha1_ctx = ctx;

    SHA1Update(sha1_ctx, data, size);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    SHA1_CTX *sha1_ctx = ctx;

    SHA1Final(result, sha1_ctx);
    free(sha1_ctx);
    return 1;
}

#elif defined(HAVE_SHA1_IN_COMMONCRYPTO)        /* Use CommonCrypto for SHA1 */

#include <CommonCrypto/CommonDigest.h>

void *
x_sha1_init(void)
{
    CC_SHA1_CTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    CC_SHA1_Init(ctx);
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    CC_SHA1_CTX *sha1_ctx = ctx;

    CC_SHA1_Update(sha1_ctx, data, size);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    CC_SHA1_CTX *sha1_ctx = ctx;

    CC_SHA1_Final(result, sha1_ctx);
    free(sha1_ctx);
    return 1;
}

#elif defined(HAVE_SHA1_IN_CRYPTOAPI)        /* Use CryptoAPI for SHA1 */

#define WIN32_LEAN_AND_MEAN
#include <X11/Xwindows.h>
#include <wincrypt.h>

static HCRYPTPROV hProv;

void *
x_sha1_init(void)
{
    HCRYPTHASH *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    CryptAcquireContext(&hProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT);
    CryptCreateHash(hProv, CALG_SHA1, 0, 0, ctx);
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    HCRYPTHASH *hHash = ctx;

    CryptHashData(*hHash, data, size, 0);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    HCRYPTHASH *hHash = ctx;
    DWORD len = 20;

    CryptGetHashParam(*hHash, HP_HASHVAL, result, &len, 0);
    CryptDestroyHash(*hHash);
    CryptReleaseContext(hProv, 0);
    free(ctx);
    return 1;
}

#elif defined(HAVE_SHA1_IN_LIBNETTLE)   /* Use libnettle for SHA1 */

#include <nettle/sha.h>

void *
x_sha1_init(void)
{
    struct sha1_ctx *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    sha1_init(ctx);
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    sha1_update(ctx, size, data);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    sha1_digest(ctx, 20, result);
    free(ctx);
    return 1;
}

#elif defined(HAVE_SHA1_IN_LIBGCRYPT)   /* Use libgcrypt for SHA1 */

#include <gcrypt.h>

void *
x_sha1_init(void)
{
    static int init;
    gcry_md_hd_t h;
    gcry_error_t err;

    if (!init) {
        if (!gcry_check_version(NULL))
            return NULL;
        gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
        gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
        init = 1;
    }

    err = gcry_md_open(&h, GCRY_MD_SHA1, 0);
    if (err)
        return NULL;
    return h;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    gcry_md_hd_t h = ctx;

    gcry_md_write(h, data, size);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    gcry_md_hd_t h = ctx;

    memcpy(result, gcry_md_read(h, GCRY_MD_SHA1), 20);
    gcry_md_close(h);
    return 1;
}

#elif defined(HAVE_SHA1_IN_LIBSHA1)     /* Use libsha1 */

#include <libsha1.h>

void *
x_sha1_init(void)
{
    sha1_ctx *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    sha1_begin(ctx);
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    sha1_hash(data, size, ctx);
    return 1;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    sha1_end(result, ctx);
    free(ctx);
    return 1;
}

#else                           /* Use OpenSSL's libcrypto */

#include <stddef.h>             /* buggy openssl/sha.h wants size_t */
#include <openssl/sha.h>

void *
x_sha1_init(void)
{
    int ret;
    SHA_CTX *ctx = malloc(sizeof(*ctx));

    if (!ctx)
        return NULL;
    ret = SHA1_Init(ctx);
    if (!ret) {
        free(ctx);
        return NULL;
    }
    return ctx;
}

int
x_sha1_update(void *ctx, void *data, int size)
{
    int ret;
    SHA_CTX *sha_ctx = ctx;

    ret = SHA1_Update(sha_ctx, data, size);
    if (!ret)
        free(sha_ctx);
    return ret;
}

int
x_sha1_final(void *ctx, unsigned char result[20])
{
    int ret;
    SHA_CTX *sha_ctx = ctx;

    ret = SHA1_Final(result, sha_ctx);
    free(sha_ctx);
    return ret;
}

#endif
