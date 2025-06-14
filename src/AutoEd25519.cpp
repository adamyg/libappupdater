//  $Id: AutoEd25519.cpp,v 1.3 2025/06/14 20:14:17 cvsuser Exp $/
// 
//  AutoUpdater: ed25519 support functions.
//
//  This file is part of libappupdater (https://github.com/adamyg/libappupdater)
//
//  Copyright (c) 2024 - 2025, Adam Young
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in all
//  copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//  SOFTWARE.
//

#include "common.h"

#include "AutoEd25519.h"

#include "../ed25519/src/ed25519.h"
#include "../util/Base64.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <cassert>
#include <errno.h>

static const char PUBLIC_KEY[] = "ED25519 PUBLIC KEY";
static const char PRIVATE_KEY[] = "ED25519 PRIVATE KEY";


static int
write_pem(FILE *fp, const char *key, const void *src, size_t length) 
{
    const uint8_t *cursor = static_cast<const uint8_t *>(src);
    char base64[70];

    fprintf(fp, "-----BEGIN %s-----\n", key);

    while (length) {
        const size_t line_length = (length > 48 ? 48 : length);

        Updater::Base64::encode(cursor, line_length, base64, sizeof(base64));
        fprintf(fp, "%s\n", base64);

        length -= line_length;
        cursor += length;
    }

    fprintf(fp, "-----END %s-----\n", key);
    memset(base64, 0, sizeof(base64));

    return !ferror(fp);
}


static bool 
read_pem(FILE *fp, const char *key, uint8_t *dst, size_t dstlength) 
{
    char line[1024];

    while (dstlength && fgets(line, sizeof(line), fp)) {
        // section makers
        if (key) { // match opening key
            if (0 != strncmp(line, "-----BEGIN ", 11) || 
                0 != strncmp(line + 11, key, strlen(key))) {
                continue;
            }
            key = NULL;
            continue;
        }

        if (0 == strncmp(line, "-----END ", 9)) {
            break; // end
        }

        // decode line
        const size_t linelen = strcspn(line, "\r\n");
        const int length =
            Updater::Base64::decode(line, linelen, dst, dstlength);

        if (-1 == length) {
            errno = EINVAL;
            return false;
        }

        dstlength -= length;
        dst += length;
    }

    if (dstlength) {
        if (key) {
            errno = EINVAL;
        } else {
            errno = ENOENT;
        }
        return false;
    }   
    return true;
}


static bool
public_key_write(FILE *strm, struct SignKeyPair *keypair)
{
    return write_pem(strm, PUBLIC_KEY, keypair->public_key, sizeof(keypair->public_key));
}


static bool
private_key_write(FILE *strm, struct SignKeyPair *keypair)
{
    return write_pem(strm, PRIVATE_KEY, keypair->private_key, sizeof(*keypair));
}


static bool
public_key_read(FILE *strm, struct SignKeyPair *keypair)
{
    return read_pem(strm, PUBLIC_KEY, keypair->public_key, sizeof(keypair->public_key));
}


static bool
private_key_read(FILE *strm, struct SignKeyPair *keypair)
{
    // base64 == 32
    //  ed25519_create_keypair(key.public_key, key.private_key, base64_data);
    // base64 == 64
    //  key.private_key + key.public_key
    // otherwise extended format
    //  Review: https://datatracker.ietf.org/doc/html/rfc7468
    //  consider: https://datatracker.ietf.org/doc/html/rfc5915
    //  
    return read_pem(strm, PRIVATE_KEY, keypair->private_key, sizeof(*keypair));
}


/////////////////////////////////////////////////////////////////////////////////////////
//  library interface
//

extern "C" {

LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
ed25519_generate_pem(const char *private_pem, const char *public_pem, const unsigned char *seed)
{
    struct SignKeyPair keypair;
    FILE *strm;

    assert(sizeof(keypair) == (ED25519_PRIVATE_LENGTH + ED25519_PUBLIC_LENGTH));
    if (NULL == private_pem || NULL == public_pem) {
        errno = EINVAL;
        return -1;
    }

    assert(sizeof(struct SignKeyPair) == (ED25519_PRIVATE_LENGTH + ED25519_PUBLIC_LENGTH));
    memset(&keypair, 0, sizeof(keypair));

    if (seed == NULL) { // keep seed? ... 3rd party code may need
        unsigned char t_seed[ED25519_SEED_LENGTH] = {0};

        ed25519_create_seed(t_seed);
        ed25519_create_keypair(keypair.public_key, keypair.private_key, t_seed);
        memset(t_seed, 0, sizeof(t_seed));
    } else {
        ed25519_create_keypair(keypair.public_key, keypair.private_key, seed);
    }

    if (NULL == (strm = fopen(private_pem, "w+"))) {
        return -1;
    }
    private_key_write(strm, &keypair);
    fclose(strm);

    if (NULL == (strm = fopen(public_pem, "w+"))) {
        return -1;
    }
    public_key_write(strm, &keypair);
    fclose(strm);

    return 0;
}


LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
ed25519_load_pem(const char *private_pem, const char *public_pem, struct SignKeyPair *keypair)
{
    FILE *strm;

    if ((NULL == public_pem && NULL == private_pem) || NULL == keypair) {
        errno = EINVAL;
        return -1;
    }

    assert(sizeof(struct SignKeyPair) == (ED25519_PRIVATE_LENGTH + ED25519_PUBLIC_LENGTH));
    memset(keypair, 0, sizeof(*keypair));

    if (private_pem) { // private + private
        if (NULL == (strm = fopen(private_pem, "r"))) {
            return -1;
        }
        const bool ret = private_key_read(strm, keypair);
        fclose(strm);
        if (! ret) {
            return -1;
        }

    } else { // public-only
        if (NULL == (strm = fopen(public_pem, "r"))) {
            return -1;
        }
        const bool ret = public_key_read(strm, keypair);
        fclose(strm);
        if (!ret) {
            return -1;
        }
    }
    return 0;
}

}; // extern "C"
  
//end
