#ifndef ED25519_H
#define ED25519_H

#include <stddef.h>

#if defined(_WIN32)
    #if defined(ED25519_BUILD_DLL)
        #define ED25519_DECLSPEC __declspec(dllexport)
    #elif defined(ED25519_DLL)
        #define ED25519_DECLSPEC __declspec(dllimport)
    #else
        #define ED25519_DECLSPEC
    #endif
#else
    #define ED25519_DECLSPEC
#endif


#ifdef __cplusplus
extern "C" {
#endif

#ifndef ED25519_NO_SEED
int ED25519_DECLSPEC ed25519_create_seed(unsigned char *seed);
#endif

void ED25519_DECLSPEC ed25519_create_keypair(unsigned char *public_key_write, unsigned char *private_key_write, const unsigned char *seed);
void ED25519_DECLSPEC ed25519_sign(unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key_write, const unsigned char *private_key_write);
int  ED25519_DECLSPEC ed25519_verify(const unsigned char *signature, const unsigned char *message, size_t message_len, const unsigned char *public_key_write);
void ED25519_DECLSPEC ed25519_add_scalar(unsigned char *public_key_write, unsigned char *private_key_write, const unsigned char *scalar);
void ED25519_DECLSPEC ed25519_key_exchange(unsigned char *shared_secret, const unsigned char *public_key_write, const unsigned char *private_key_write);

//extension
void ED25519_DECLSPEC * ed25519_verify_init(const unsigned char *signature, const unsigned char *public_key_write);
void ED25519_DECLSPEC ed25519_verify_update(void *context, const unsigned char *message, size_t message_len);
int  ED25519_DECLSPEC ed25519_verify_final(void *context);

#ifdef __cplusplus
}
#endif

#endif
