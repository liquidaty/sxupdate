#include <stdio.h>
#include "log.h"

int sxupdate_printerr(const char *format, ...) {
  int rc;
  va_list args;
  va_start(args, format);
  rc = vfprintf(stderr, format, args);
  va_end(args);
  rc += fprintf(stderr, "\n");
  return rc;
}

void sxupdate_verbose(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fprintf(stderr, "\n");
}
