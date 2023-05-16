#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "parse.h"
#include "verify.h"

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
    else if(prop_name && !strcmp(prop_name, "prerelease"))
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
      err = fprintf(stderr, "Warning! invalid integer value ignored\n"); // to do: use custom error handler
    else if(sz_target && i < 0)
      err = fprintf(stderr, "Warning! invalid integer (size_t) value ignored: %lli\n", i); // to do: use custom error handler
    else if(int_target)
      *int_target = (int)i;
    else if(sz_target)
      *sz_target = (size_t)i;
    if(err) {
      char *s = NULL;
      json_value_to_string_dup(value, &s, 1);
      if(s)
        fprintf(stderr, "Value on error: %s\n", s);
      free(s);
    }
  }
  return 1; // 1 = continue; 0 = halt
}

/* simple helper to check for case-insensitive ascii suffix */
static int str_ends_with(const char *s, const char *suffix) {
  if(!s) return 0;
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

int sxupdate_url_is_file(const char *s) {
  if(!s) return 0;
  size_t len = strlen(s);
  if(len > 8 && !memcmp(s, SXUPDATE_FILE_PREFIX, 7))
    return 1;
  return 0;
}

int sxupdate_url_is_https(const char *s) {
  if(!s) return 0;
  size_t len = strlen(s);
  if(len > 9 && !memcmp(s, SXUPDATE_HTTPS_PREFIX, 8))
    return 1;
  return 0;
}

/**
 * see schema/appcast.schema.json
 * return 1 if s only contains alphanumeric characters, dash, slash, underscore and period",
 **/
int sxupdate_is_relative_filename(const char *s) {
  if(!s) return 0;
  size_t len = s ? strlen(s) : 0;
  if(!len)
    return 0;
  for(size_t i = 0; i < len; i++) {
    char c = s[i];
    if(!((c >= 'a' && c <= 'z')
         || (c >= 'A' && c <= 'Z')
         || (c >= '0' && c <= '9')
         || (c == '_' || c == '-' || c == '/' || c == '.')
         )
       )
      return 0;
  }
  if(strstr(s, "//") || s[len-1] == '/')
    return 0;
  return 1;
}


/***
 * sxupdate_parse_ok: return 1 if ok, 0 if NOT OK
 *
 * TO DO: use custom error handler
 */
static int sxupdate_parse_ok(sxupdate_t handle) {
  struct sxupdate_version *v = &handle->latest_version;
  int err = 0;

  // check filename
  if(!v->enclosure.filename || !*v->enclosure.filename)
    err = fprintf(stderr, "Version enclosure: missing filename\n");
  else if(str_ends_with(v->enclosure.filename, ".exe"))
    // if filename ends with .exe, remove that suffix
    v->enclosure.filename[strlen(v->enclosure.filename) - 4] = '\0';

  // check url
  if(!v->enclosure.url)
    err = fprintf(stderr, "Version enclosure: missing url\n");

  if(v->enclosure.url
     && !sxupdate_url_is_https(v->enclosure.url)
     && !sxupdate_url_is_file(v->enclosure.url)
     && !sxupdate_is_relative_filename(v->enclosure.url))
    err = fprintf(stderr, "Version enclosure: bad url (%s)\n", v->enclosure.url);

  // check major / minor / patch
  if(v->version.major < 0 || v->version.minor < 0 || v->version.patch < 0)
    err = fprintf(stderr, "Invalid or unspecified version major, minor and/or patch\n");

  if(!err) {
    if(!handle->no_public_key) {
      if(!v->enclosure.signature)
        err = fprintf(stderr, "Version enclosure: missing signature\n");
      else {
        if(sxupdate_set_signature_from_b64(handle, v->enclosure.signature)
           != sxupdate_status_ok)
          err = fprintf(stderr, "Version enclosure: unable to convert signature from base64\n");
      }
    } else if(v->enclosure.signature)
      err = fprintf(stderr, "Version is signed, but no public key provided to verify\n");
  }
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
