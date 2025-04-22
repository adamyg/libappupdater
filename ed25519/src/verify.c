#include "ed25519.h"
#include "sha512.h"
#include "ge.h"
#include "sc.h"

static int consttime_equal(const unsigned char *x, const unsigned char *y) {
    unsigned char r = 0;

    r = x[0] ^ y[0];
    #define F(i) r |= x[i] ^ y[i]
    F(1);
    F(2);
    F(3);
    F(4);
    F(5);
    F(6);
    F(7);
    F(8);
    F(9);
    F(10);
    F(11);
    F(12);
    F(13);
    F(14);
    F(15);
    F(16);
    F(17);
    F(18);
    F(19);
    F(20);
    F(21);
    F(22);
    F(23);
    F(24);
    F(25);
    F(26);
    F(27);
    F(28);
    F(29);
    F(30);
    F(31);
    #undef F

    return !r;
}

int ed25519_verify(const unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key_write) {
    unsigned char h[64];
    unsigned char checker[32];
    sha512_context hash;
    ge_p3 A;
    ge_p2 R;

    if (signature[63] & 224) {
        return 0;
    }

    if (ge_frombytes_negate_vartime(&A, public_key_write) != 0) {
        return 0;
    }

    sha512_init(&hash);
    sha512_update(&hash, signature, 32);
    sha512_update(&hash, public_key_write, 32);
    sha512_update(&hash, message, message_len);
    sha512_final(&hash, h);
    
    sc_reduce(h);
    ge_double_scalarmult_vartime(&R, h, &A, signature + 32);
    ge_tobytes(checker, &R);

    if (!consttime_equal(checker, signature)) {
        return 0;
    }

    return 1;
}

/////////////////////////////////////////////////////////////////////////////////////////

#include <stdlib.h>
#include <string.h>

struct VerifyContext {
    const unsigned char *signature;
    sha512_context hash;
};


void *
ed25519_verify_init(const unsigned char *signature, const unsigned char *public_key_write)
{
    struct VerifyContext *ctx;

    if (signature[63] & 224) {
        return NULL;
    }

    if (NULL == (ctx = malloc(sizeof(*ctx)))) {
        return NULL;
    }

    ctx->signature = signature;

    sha512_init(&ctx->hash);
    sha512_update(&ctx->hash, signature, 32);
    sha512_update(&ctx->hash, public_key_write, 32);
    ctx->signature = signature;

    return (void *)ctx;
}


void 
ed25519_verify_update(void *context, const unsigned char *message, size_t message_len)
{
    if (NULL == context) {
        return;
    }

    struct VerifyContext *ctx = (struct VerifyContext *)context;
    sha512_update(&ctx->hash, message, message_len);
}


int
ed25519_verify_final(void *context)
{
    unsigned char h[64];
    unsigned char checker[32];
    ge_p3 A;
    ge_p2 R;

    if (NULL == context) {
        return 0;
    }

    struct VerifyContext *ctx = (struct VerifyContext *)context;
    sha512_final(&ctx->hash, h);

    sc_reduce(h);
    ge_double_scalarmult_vartime(&R, h, &A, ctx->signature + 32);
    ge_tobytes(checker, &R);

    const int ret = !consttime_equal(checker, ctx->signature);

    memset(ctx, 0, sizeof(*ctx));
    free((void *)ctx);

    return ret;
}

//
