#ifndef SXUPDATE_INTERNAL_H
#define SXUPDATE_INTERNAL_H

#ifndef NO_SIGNATURE
#include "openssl/rsa.h"
#endif
#include "../include/api.h"
#include <yajl_helper/yajl_helper.h>



struct sxupdate_data {
  struct {
    yajl_status stat;
    struct yajl_helper_parse_state st;
  } parser;
  struct sxupdate_semantic_version (*get_current_version)();
  enum sxupdate_action (*sync_cb)(const struct sxupdate_version *version);

  char *url;

  struct curl_slist *http_headers;
  struct sxupdate_version latest_version;

#ifndef NO_SIGNATURE
  RSA *public_key;
  struct {
    unsigned char *signature; // binary value of latest_version.signature
    size_t signature_length;
  } latest_version_internal;
#endif

  unsigned char verbosity;

  unsigned char url_is_file:1;
  unsigned char no_public_key:1;
  unsigned char _:6;
};


#endif
