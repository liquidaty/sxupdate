#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "log.h"

static size_t version_prerelease_next_identifier_len(char *s) {
  char *dot = strchr(s, '.');
  if(!dot)
    dot = strchr(s, '\0');
  else
    *dot = '\0'; // to allow strcmp on this segment
  return dot - s;
}

static int version_prerelease_numeric(const char *s, size_t len) {
  for(size_t i = 0; i < len; i++)
    if(!strchr("0123456789", s[i]))
      return 0;
  return 1;
}

/* compare two prerelease strings. See https://semver.org/
   this will modify the input strings!
 */
static int version_prerelease_cmp(char *x, char *y) {
  char *this_x = x;
  char *this_y = y;

  int result = 0;
  while(result == 0) {
    size_t xid_len = version_prerelease_next_identifier_len(this_x);
    size_t yid_len = version_prerelease_next_identifier_len(this_y);
    if(xid_len == 0 && yid_len == 0) return 0;
    if(yid_len == 0) return 1;
    if(xid_len == 0) return -1;

    if(xid_len == yid_len && !memcmp(this_x, this_y, xid_len)) { // they are equal
      this_x += xid_len + 1;
      this_y += yid_len + 1;
      continue;
    }

    // if we're here, they are not equal
    char x_is_numeric = version_prerelease_numeric(this_x, xid_len);
    char y_is_numeric = version_prerelease_numeric(this_y, yid_len);

    if(y_is_numeric && !x_is_numeric) return 1;
    if(!y_is_numeric && x_is_numeric) return -1;
    if(!y_is_numeric && !x_is_numeric) return strcmp(this_x, this_y);

    // they are both numeric
    long x_i, y_i;
    x_i = strtol(this_x, NULL, 10);
    y_i = strtol(this_y, NULL, 10);
    result = x_i > y_i ? 1 : -1;
  }
  return result;
}

/* compare two versions. return 1 if v1 > v2, -1 if v1 < v2, or 0 if they are equal */
int sxupdate_version_cmp(struct sxupdate_semantic_version v1, struct sxupdate_semantic_version v2, unsigned char verbose) {
  if(verbose)
    sxupdate_verbose("comparing versions: v1 = %i.%i.%i-%s; v2 = %i.%i.%i-%s",
            v1.major, v1.minor, v1.patch, v1.prerelease ? v1.prerelease : "",
            v2.major, v2.minor, v2.patch, v2.prerelease ? v2.prerelease : ""
            );
#define SXUPDATE_VERSION_CMP_EXIT(part, rc) do { if(verbose) sxupdate_verbose("result " #part " = %i", rc); return rc; } while(0)

#define SXUPDATE_VERSION_CMP_VALUE(X) \
  if(v1.X > v2.X) SXUPDATE_VERSION_CMP_EXIT(X, 1); \
  if(v1.X < v2.X) SXUPDATE_VERSION_CMP_EXIT(X, -1)

  SXUPDATE_VERSION_CMP_VALUE(major);
  SXUPDATE_VERSION_CMP_VALUE(minor);
  SXUPDATE_VERSION_CMP_VALUE(patch);

  if(!v1.prerelease && v2.prerelease) SXUPDATE_VERSION_CMP_EXIT(prerelease, 1);
  if(!v2.prerelease && v1.prerelease) SXUPDATE_VERSION_CMP_EXIT(prerelease, -1);

  if(v1.prerelease && v2.prerelease && strcmp(v1.prerelease, v2.prerelease)) {
    char *pr1 = strdup(v1.prerelease);
    char *pr2 = strdup(v2.prerelease);
    int rc = 0;
    if(!(pr1 && pr2))
      sxupdate_printerr("Out of memory!");
    else
      rc = version_prerelease_cmp(pr1, pr2);
    free(pr1);
    free(pr2);
    SXUPDATE_VERSION_CMP_EXIT(prerelease, rc);
  }
  SXUPDATE_VERSION_CMP_EXIT("", 0);
}

void sxupdate_version_free(struct sxupdate_version *v) {
  free(v->title);
  free(v->link);
  free(v->description);
  free(v->pubDate);

  free(v->version.prerelease);
  free(v->version.meta);

  free(v->enclosure.url);
  free(v->enclosure.type);
  free(v->enclosure.signature);
  free(v->enclosure.filename);
}
