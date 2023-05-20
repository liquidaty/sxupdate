#ifndef SXUPDATE_INTERNAL_H
#define SXUPDATE_INTERNAL_H

#ifndef NO_SIGNATURE
#include "openssl/rsa.h"
#endif
#include "../include/api.h"
#include <yajl_helper/yajl_helper.h>

struct sxupdate_string_list {
  struct sxupdate_string_list *next;
  char *value;
};

struct sxupdate_data {
  struct {
    yajl_status stat;
    struct yajl_helper_parse_state st;
    size_t scanned_bytes;
  } parser;
  struct sxupdate_semantic_version (*get_current_version)();
  sxupdate_interaction_handler interaction_handler;
  enum sxupdate_step step; // what step we are currently processing

  char *url;
  struct sxupdate_string_list *installer_args, **installer_args_next;

  struct curl_slist *http_headers;
  long http_code; // curl response
  struct sxupdate_version latest_version;

#ifndef NO_SIGNATURE
  RSA *public_key;
  struct {
    unsigned char *signature; // binary value of latest_version.signature
    size_t signature_length;
  } latest_version_internal;
#endif

  const char *err_msg;

  unsigned char verbosity;

  unsigned char url_is_file:1;
  unsigned char no_public_key:1;
  unsigned char got_version:1;
  unsigned char _:5;
};


#endif
