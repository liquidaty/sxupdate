
static int sxupdate_start_map(struct yajl_helper_parse_state *st) {
}

static int sxupdate_end_map(struct yajl_helper_parse_state *st) {
}

static int sxupdate_map_key(struct yajl_helper_parse_state *st,
                             const unsigned char *value, size_t len) {
}

static int sxupdate_end_array(struct yajl_helper_parse_state *st) {
}

static int sxupdate_process_value(struct yajl_helper_parse_state *st, struct json_value *value) {

}

enum yajl_status sxupdate_parse(sxupdater *sxu, const char *data, size_t len) {
  sxu->stat =
    yajl_helper_parse_state_init(&sxu->st, 32,
                                 sxupdate_start_map,
                                 sxupdate_end_map,
                                 sxupdate_map_key,
                                 NULL, // sted_start_array,
                                 sxupdate_end_array,
                                 sxupdate_process_value,
                                 sxu);
  if(sxu->stat == yajl_status_ok)
    sxu->stat = yajl_parse(sxu->st.yajl, data, len);
  return sxu->stat;
}
