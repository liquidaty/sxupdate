#ifndef SXUPDATE_VERIFY_H
#define SXUPDATE_VERIFY_H

#include <openssl/rsa.h>

RSA *sxupdate_public_key_from_pem_file(const char *filepath);

enum sxupdate_status sxupdate_set_signature_from_b64(sxupdate_t handle,
                                                     const char *b64);

enum sxupdate_status sxupdate_verify_signature(sxupdate_t handle, const char *filepath);

#endif
