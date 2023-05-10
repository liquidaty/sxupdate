#ifndef SXUPDATE_INTERNAL_H
#define SXUPDATE_INTERNAL_H

struct sxupdate_data {
};

struct sxupdate_parser {
  yajl_status stat;
  struct yajl_helper_parse_state st;
  struct sted_spec_parse_data data;
};


#endif
