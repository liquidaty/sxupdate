#ifndef SXUPDATE_PARSE_H
#define SXUPDATE_PARSE_H

#include "../include/api.h"
#include "internal.h"

#define  SXUPDATE_HTTPS_PREFIX "https://"
#define  SXUPDATE_FILE_PREFIX "file://"

enum sxupdate_status sxupdate_parse_init(sxupdate_t handle);

/***
 * Parse a chunk of metadata (JSON)
 * @param sxu : sxupdate handle
 * @param data: JSON data (bytes)
 * @param len : length (in bytes) of data
 *
 * @return: 0 on success, non-zero otherwise
 */
enum sxupdate_status sxupdate_parse(sxupdate_t handle, const char *data, size_t len);

enum sxupdate_status sxupdate_parse_finish(sxupdate_t handle);

int sxupdate_url_is_file(const char *s);
int sxupdate_url_is_https(const char *s);

/**
 * see schema/appcast.schema.json
 * return 1 if s only contains alphanumeric characters, dash, underscore and period",
 **/
int sxupdate_is_relative_filename(const char *s);

#endif
