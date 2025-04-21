#ifndef AUTOED25519_H_INCLUDED
#define AUTOED25519_H_INCLUDED
//  $Id: AutoEd25519.h,v 1.1 2025/04/21 13:59:08 cvsuser Exp $/
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

#include "AutoLinkage.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define ED25519_SEED_LENGTH 32
#define ED25519_SIGNATURE_LENGTH 64
#define ED25519_PRIVATE_LENGTH 64
#define ED25519_PUBLIC_LENGTH 32

struct SignKeyPair {
    unsigned char private_key[ED25519_PRIVATE_LENGTH];
    unsigned char public_key[ED25519_PUBLIC_LENGTH];
//  unsigned char version;
};

LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
ed25519_generate_pem(const char *private_pem, const char *public_pem, const unsigned char *seed);

LIBAUTOUPDATER_LINKAGE int LIBAUTOUPDATER_ENTRY
ed25519_load_pem(const char *private_pem, const char *public_pem, struct SignKeyPair *keypair);

#if defined(__cplusplus)
}; // extern "C"
#endif

#endif //AUTOED25519_H_INCLUDED
  
//end
