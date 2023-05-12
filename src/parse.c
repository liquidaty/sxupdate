#include <string.h>
#include <stdint.h>

#include "parse.h"

static int sxupdate_process_value(struct yajl_helper_parse_state *st, struct json_value *value) {
  sxupdate_t handle = yajl_helper_data(st);
  char **str_target = NULL;
  int *int_target = NULL;
  size_t *sz_target = NULL;

  struct sxupdate_version *v = &handle->latest_version;
  const char *prop_name = yajl_helper_get_map_key(st, 0);

  if(yajl_helper_got_path(st, 3, "{items[{")) {
    if(prop_name && !strcmp(prop_name, "title"))
      str_target = &v->title;
    else if(prop_name && !strcmp(prop_name, "link"))
      str_target = &v->link;
    else if(prop_name && !strcmp(prop_name, "description"))
      str_target = &v->description;
    else if(prop_name && !strcmp(prop_name, "pubDate"))
      str_target = &v->pubDate;
  } else if(yajl_helper_got_path(st, 4, "{items[{version{")) {
    if(prop_name && !strcmp(prop_name, "major"))
      int_target = &v->version.major;
    else if(prop_name && !strcmp(prop_name, "minor"))
      int_target = &v->version.minor;
    else if(prop_name && !strcmp(prop_name, "patch"))
      int_target = &v->version.patch;
    else if(prop_name && !strcmp(prop_name, "prelease"))
      str_target = &v->version.prerelease;
    else if(prop_name && !strcmp(prop_name, "meta"))
      str_target = &v->version.meta;
  } else if(yajl_helper_got_path(st, 4, "{items[{enclosure{")) {
    if(prop_name && !strcmp(prop_name, "url"))
      str_target = &v->enclosure.url;
    else if(prop_name && !strcmp(prop_name, "length"))
      sz_target = &v->enclosure.length;
    else if(prop_name && !strcmp(prop_name, "type"))
      str_target = &v->enclosure.type;
    else if(prop_name && !strcmp(prop_name, "signature"))
      str_target = &v->enclosure.signature;
    else if(prop_name && !strcmp(prop_name, "filename"))
      str_target = &v->enclosure.filename;
  }

  if(str_target)
    json_value_to_string_dup(value, str_target, 1);
  else if(int_target || sz_target) {
    int err;
    long long i = json_value_long(value, &err);
    if(int_target && (i < 0 || i >= INTMAX_MAX))
      fprintf(stderr, "Warning! invalid integer value ignored\n"); // to do: use custom error handler
    else if(sz_target && (i < 0 || i >= (long long int)SIZE_MAX))
      fprintf(stderr, "Warning! invalid integer value ignored\n"); // to do: use custom error handler
    else if(int_target)
      *int_target = (int)i;
    else if(sz_target)
      *sz_target = (size_t)i;
  }
  return 1; // 1 = continue; 0 = halt
}

/* simple helper to check for case-insensitive ascii suffix */
static int str_ends_with(const char *s, const char *suffix) {
  size_t len = s ? strlen(s) : 0;
  if(len > strlen(suffix)) {
    size_t j = 0;
    size_t i = len - strlen(suffix);
    for(; i < len; i++, j++)
      if(tolower(s[i]) != tolower(suffix[j]))
        break;
    if(i == len)
      return 1;
  }
  return 0;
}

/* check_url: return 0 if bad; 1 if https, 2 if file */
int sxupdate_url_ok(const char *s) {
  size_t len = strlen(s);
  if(len > 8 && !memcmp(s, "file://", 7))
    return 2; // it's a file
  if(len < 9 || memcmp(s, "https://", 8))
    return 0; // bad!
  return 1; // ok url
}

/***
 * sxupdate_parse_ok: return 1 if ok, 0 if NOT OK
 *
 * TO DO: use custom error handler
 */
static int sxupdate_parse_ok(sxupdate_t handle) {
  struct sxupdate_version *v = &handle->latest_version;
  int err = 0;

  // if filename ends with .exe, remove that suffix
  if(str_ends_with(v->enclosure.filename, ".exe"))
    v->enclosure.filename[strlen(v->enclosure.filename) - 4] = '\0';

  // check filename
  if(!v->enclosure.filename || !*v->enclosure.filename)
    err = fprintf(stderr, "Version enclosure: missing filename\n");

  // check url
  if(!v->enclosure.url)
    err = fprintf(stderr, "Version enclosure: missing url\n");

  if(!(sxupdate_url_ok(v->enclosure.url)))
    err = fprintf(stderr, "Version enclosure: bad url (%s)\n", v->enclosure.url);

  // check major / minor / patch
  if(v->version.major < 0 || v->version.minor < 0 || v->version.patch < 0)
    err = fprintf(stderr, "Invalid or unspecified version major, minor and/or patch\n");

  // TO DO: check signature!

  if(err)
    return 0; // not ok

  return 1; // ok
}

/***
 * Parse a chunk of metadata (JSON)
 * @param sxu : sxupdate handle
 * @param data: JSON data (bytes)
 * @param len : length (in bytes) of data
 *
 * @return: 0 on success, non-zero otherwise
 */
enum sxupdate_status sxupdate_parse(sxupdate_t handle, const char *data, size_t len) {
  if(handle->parser.stat == yajl_status_ok
     && (handle->parser.stat = yajl_parse(handle->parser.st.yajl, (const unsigned char *)data, len)) == yajl_status_ok)
    return sxupdate_status_ok;
  return sxupdate_status_error;
}

enum sxupdate_status sxupdate_parse_finish(sxupdate_t handle) {
  if(handle->parser.stat == yajl_status_ok
     && (handle->parser.stat = yajl_complete_parse(handle->parser.st.yajl)) == yajl_status_ok
     && sxupdate_parse_ok(handle))
    return sxupdate_status_ok;
  return sxupdate_status_error;
}

enum sxupdate_status sxupdate_parse_init(sxupdate_t handle) {
  handle->parser.stat =
    yajl_helper_parse_state_init(&handle->parser.st, 32,
                                 NULL, // start_map,
                                 NULL, // end_map,
                                 NULL, // map_key,
                                 NULL, // start_array,
                                 NULL, // end_array,
                                 sxupdate_process_value,
                                 handle);

  /* initialize major/minor/patch to -1, so we know after parsing whether it was explicitly set to zero */
  handle->latest_version.version.major =
    handle->latest_version.version.minor =
    handle->latest_version.version.patch = -1;
  return handle->parser.stat == yajl_status_ok ? sxupdate_status_ok : sxupdate_status_error;
}
