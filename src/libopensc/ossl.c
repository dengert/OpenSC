/*
 * OpenSSL helper functions to deal with libctx
 *
 * Copyright (C) 2023 Simo Sorce <simo@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#ifdef ENABLE_OPENSSL		/* empty file without openssl */

#include "libopensc/opensc.h"

EVP_MD *sc_evp_md(struct sc_context *context, const char *algorithm)
{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
    return (EVP_MD *)EVP_get_digestbyname(algorithm);
#else
    return EVP_MD_fetch(context->osslctx, algorithm, NULL);
#endif
}

void sc_evp_md_free(EVP_MD *md)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    EVP_MD_free(md);
#endif
    return;
}

EVP_PKEY_CTX *sc_evp_pkey_ctx_new(struct sc_context *context, EVP_PKEY *pkey)
{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
    return EVP_PKEY_CTX_new(pkey, NULL);
#else
    return EVP_PKEY_CTX_new_from_pkey(context->osslctx, pkey, NULL);
#endif
}

EVP_CIPHER *sc_evp_cipher(struct sc_context *context, const char *algorithm)
{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
    return (EVP_CIPHER *)EVP_get_cipherbyname(algorithm);
#else
    return EVP_CIPHER_fetch(context->osslctx, algorithm, NULL);
#endif
}

void sc_evp_cipher_free(EVP_CIPHER *cipher)
{
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
    EVP_CIPHER_free(cipher);
#endif
    return;
}

#else
void *sc_evp_md(struct sc_context *context, const char *algorithm)
{
    return NULL;
}
void sc_evp_md_free(void *md)
{
    return;
}
void *sc_evp_pkey_ctx_new(struct sc_context *context, void *pkey)
{
    return NULL;
}
void *sc_evp_cipher(struct sc_context *context, const char *algorithm)
{
    return NULL;
}
void sc_evp_cipher_free(void *cipher)
{
    return;
}
#endif
