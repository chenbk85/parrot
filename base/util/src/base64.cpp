/* ====================================================================
 * Copyright (c) 1995-1999 The Apache Group.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * 4. The names "Apache Server" and "Apache Group" must not be used to
 *    endorse or promote products derived from this software without
 *    prior written permission. For written permission, please contact
 *    apache@apache.org.
 *
 * 5. Products derived from this software may not be called "Apache"
 *    nor may "Apache" appear in their names without prior written
 *    permission of the Apache Group.
 *
 * 6. Redistributions of any form whatsoever must retain the following
 *    acknowledgment:
 *    "This product includes software developed by the Apache Group
 *    for use in the Apache HTTP server project (http://www.apache.org/)."
 *
 * THIS SOFTWARE IS PROVIDED BY THE APACHE GROUP ``AS IS'' AND ANY
 * EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE APACHE GROUP OR
 * ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * ====================================================================
 *
 * This software consists of voluntary contributions made by many
 * individuals on behalf of the Apache Group and was originally based
 * on public domain software written at the National Center for
 * Supercomputing Applications, University of Illinois, Urbana-Champaign.
 * For more information on the Apache Group and the Apache HTTP server
 * project, please see <http://www.apache.org/>.
 *
 */

/* base64 encoder/decoder. Originally part of main/util.c
 * but moved here so that support/ab and ap_sha1.c could
 * use it. This meant removing the ap_palloc()s and adding
 * ugly 'len' functions, which is quite a nasty cost.
 */

#include "base64.h"

/* aaaack but it's fast and const should make it shared text page. */
static const unsigned char pr2six[256] = {
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 62, 64, 64, 64, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60,
    61, 64, 64, 64, 64, 64, 64, 64, 0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
    11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64,
    64, 64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42,
    43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64};

static const char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

namespace parrot
{
uint32_t getBase64DecodeLen(const char* bufcoded)
{
    uint32_t nbytesdecoded;
    const unsigned char* bufin;
    uint32_t nprbytes;

    bufin = (const unsigned char*)bufcoded;
    while (pr2six[*(bufin++)] <= 63)
        ;

    nprbytes = (bufin - (const unsigned char*)bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}

uint32_t base64Decode(char* out, const char* bufcoded)
{
    uint32_t nbytesdecoded;
    const unsigned char* bufin;
    unsigned char* bufout;
    uint32_t nprbytes;

    bufin = (const unsigned char*)bufcoded;
    while (pr2six[*(bufin++)] <= 63)
        ;
    nprbytes = (bufin - (const unsigned char*)bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char*)out;
    bufin = (const unsigned char*)bufcoded;

    while (nprbytes > 4)
    {
        *(bufout++) =
            (unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
        *(bufout++) = (unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1)
    {
        *(bufout++) =
            (unsigned char)(pr2six[*bufin] << 2 | pr2six[bufin[1]] >> 4);
    }
    if (nprbytes > 2)
    {
        *(bufout++) =
            (unsigned char)(pr2six[bufin[1]] << 4 | pr2six[bufin[2]] >> 2);
    }
    if (nprbytes > 3)
    {
        *(bufout++) = (unsigned char)(pr2six[bufin[2]] << 6 | pr2six[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;

    return nbytesdecoded;
}

uint32_t getBase64EncodeLen(uint32_t len)
{
    return ((len + 2) / 3 * 4) + 1;
}

uint32_t base64Encode(char* bufout, const char* in, uint32_t len)
{
    uint32_t i;
    char* p;
    const unsigned char* str = (const unsigned char*)in;

    p = bufout;
    for (i = 0; i < len - 2; i += 3)
    {
        *p++ = basis_64[(str[i] >> 2) & 0x3F];
        *p++ =
            basis_64[((str[i] & 0x3) << 4) | ((int)(str[i + 1] & 0xF0) >> 4)];
        *p++ = basis_64[((str[i + 1] & 0xF) << 2) |
                        ((int)(str[i + 2] & 0xC0) >> 6)];
        *p++ = basis_64[str[i + 2] & 0x3F];
    }
    if (i < len)
    {
        *p++ = basis_64[(str[i] >> 2) & 0x3F];
        if (i == (len - 1))
        {
            *p++ = basis_64[((str[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else
        {
            *p++ = basis_64[((str[i] & 0x3) << 4) |
                            ((int)(str[i + 1] & 0xF0) >> 4)];
            *p++ = basis_64[((str[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - bufout;
}
}
