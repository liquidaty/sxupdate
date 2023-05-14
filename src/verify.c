#include "internal.h"
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
// #include <openssl/evp.h>
#include <string.h>

/**
 * return 1 on success, 0 on failure
 */
static int verify_signature(const char *filename, RSA *public_key, unsigned char *signature, unsigned int signature_length) {
  FILE *file = fopen(filename, "rb");
  fseek(file, 0, SEEK_END);
  long filesize = ftell(file);
  rewind(file);
  unsigned char *buffer = malloc(filesize);
  fread(buffer, filesize, 1, file);
  fclose(file);

  unsigned char hash[SHA256_DIGEST_LENGTH];
  SHA256(buffer, filesize, hash);

  int result = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH, signature, signature_length, public_key);

  free(buffer);
  return result;
}

static unsigned char *base64_decode(const char *input, int length, int *outlen) {
  BIO *b64, *bmem;

  unsigned char *buffer = (unsigned char*)malloc(length);
  memset(buffer, 0, length);

  b64 = BIO_new(BIO_f_base64());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL); // Ignore newlines

  bmem = BIO_new_mem_buf(input, length);
  bmem = BIO_push(b64, bmem);

  *outlen = BIO_read(bmem, buffer, length);

  BIO_free_all(bmem);

  return buffer;
}

enum sxupdate_status sxupdate_set_signature_from_b64(sxupdate_t handle,
                                                     const char *b64) {
  if(handle->verbosity)
    fprintf(stderr, "Converting signature from base64: %s\n...", b64);
  free(handle->latest_version_internal.signature);

  /* convert from base64 */
  int outlen_i = 0;
  handle->latest_version_internal.signature = base64_decode(b64, strlen(b64), &outlen_i);
  if(outlen_i > 0) {
    handle->latest_version_internal.signature_length = (size_t)outlen_i;
    if(handle->verbosity)
      fprintf(stderr, "Success!\n");
    return sxupdate_status_ok;
  }

  if(handle->verbosity)
    fprintf(stderr, "Error!!!\n");
  free(handle->latest_version_internal.signature);
  handle->latest_version_internal.signature = NULL;
  return sxupdate_status_error;
}

RSA *sxupdate_public_key_from_pem_file(const char *filepath) {
  FILE *pubkey_file = fopen(filepath, "r");
  if (pubkey_file == NULL) {
    perror(filepath);
    return NULL;
  }

  RSA *public_key = RSA_new();
  public_key = PEM_read_RSA_PUBKEY(pubkey_file, &public_key, NULL, NULL);
  fclose(pubkey_file);

  if(!public_key)
    fprintf(stderr, "Unable to load public key from %s\n", filepath);

  return public_key;
}

enum sxupdate_status sxupdate_verify_signature(sxupdate_t handle, const char *filepath) {
  if(handle->verbosity)
    fprintf(stderr, "Verifying the signature for %s\n...", filepath);
  if(!(handle->public_key && !handle->no_public_key))
    return sxupdate_status_ok; // no signature check

  if(verify_signature(filepath, handle->public_key, handle->latest_version_internal.signature, handle->latest_version_internal.signature_length)) {
    if(handle->verbosity)
      fprintf(stderr, "OK!\n");
    return sxupdate_status_ok;
  }

  fprintf(stderr, "Signature verification failed!!\n");
  return sxupdate_status_error;
}
