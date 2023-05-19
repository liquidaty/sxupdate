#ifndef SXUPDATE_LOG_H
#define SXUPDATE_LOG_H

#include <stdarg.h>

int sxupdate_printerr(const char *format, ...);
void sxupdate_verbose(const char *format, ...);

#endif
