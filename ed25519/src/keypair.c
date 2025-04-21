#include "ed25519.h"
#include "sha512.h"
#include "ge.h"


void ed25519_create_keypair(unsigned char *public_key_write, unsigned char *private_key_write, const unsigned char *seed) {
    ge_p3 A;

    sha512(seed, 32, private_key_write);
    private_key_write[0] &= 248;
    private_key_write[31] &= 63;
    private_key_write[31] |= 64;

    ge_scalarmult_base(&A, private_key_write);
    ge_p3_tobytes(public_key_write, &A);
}
