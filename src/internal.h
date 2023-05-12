#ifndef SXUPDATE_INTERNAL_H
#define SXUPDATE_INTERNAL_H

#include "../include/api.h"
#include <yajl_helper.h>

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

  unsigned char url_is_file:1;
  unsigned char _:7;
};


#endif
