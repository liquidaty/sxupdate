#ifndef SXUPDATE_PARSE_H
#define SXUPDATE_PARSE_H

#include "../include/api.h"
#include "internal.h"

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

/* check url: return 0 if bad; 1 if https, 2 if file */
int sxupdate_url_ok(const char *s);


#endif
