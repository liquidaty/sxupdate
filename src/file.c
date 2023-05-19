#include <stdio.h>
#include <string.h>
#include <stdlib.h> // getenv, mkstemp
#include <errno.h>
#include <unistd.h> // for close()
#include <sys/stat.h>

#if defined(_WIN32) || defined(WIN32) || defined(WIN)
#include <windows.h>
#endif

#include "log.h"

/**
 * Check if a directory exists
 * return true (non-zero) or false (zero)
 */
static int dir_exists(const char *path) {
  struct stat path_stat;
  if(!stat(path, &path_stat))
    return S_ISDIR(path_stat.st_mode);
  return 0;
}

/**
 * Check if a file exists
 * return true (non-zero) or false (zero)
 */
static int file_exists(const char* filename) {
#if defined(_WIN32) || defined(WIN32) || defined(WIN)
  DWORD attributes = GetFileAttributes(filename);
  return (attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY));
#else
  struct stat buffer;
  if(stat(filename, &buffer) == 0) {
    char is_dir = buffer.st_mode & S_IFDIR ? 1 : 0;
    if(!is_dir)
      return 1;
  }
  return 0;
#endif
}

#define SXUPDATE_GET_INSTALLER_PATH_MAX_TRIES 10000
#define SXUPDATE_GET_INSTALLER_PATH_MAX_TRIES_MIN_STR_LEN 5
char *sxupdate_get_installer_download_path(const char *basename) {
  char *tmpdir;
  char slash;
  const char *suffix;
#if defined(_WIN32) || defined(WIN32) || defined(WIN)
  slash = '\\';
  suffix = ".exe";
  tmpdir = getenv("TEMP");
  if(!tmpdir)
    tmpdir = getenv("TMP");
  if(!tmpdir)
    tmpdir = ".";
#else
  slash = '/';
  suffix = "";
  tmpdir = getenv("TMPDIR");
  if(!tmpdir)
    tmpdir = "/tmp";
#endif

  if(!dir_exists(tmpdir)) {
    sxupdate_printerr("Could not find temporary directory %s", tmpdir);
    return NULL;
  }

  size_t len = strlen(tmpdir) + strlen(basename) + strlen(suffix) + SXUPDATE_GET_INSTALLER_PATH_MAX_TRIES_MIN_STR_LEN + 10;
  char *s = calloc(1, len + 1);
  if(!s) {
    sxupdate_printerr("Out of memory!");
    return NULL;
  }

  for(int i = 0; i < SXUPDATE_GET_INSTALLER_PATH_MAX_TRIES; i++) { // overusing malloc() but root of evil etc...
    if(i == 0)
      snprintf(s, len, "%s%c%s%s", tmpdir, slash, basename, suffix);
    else
      snprintf(s, len, "%s%c%s (%i)%s", tmpdir, slash, basename, i, suffix);
    if(!file_exists(s))
      return s;
  }
  free(s);
  sxupdate_printerr("Unable to find an unused filename after %i tries with base:\n  %s%c%s",
          SXUPDATE_GET_INSTALLER_PATH_MAX_TRIES, tmpdir, slash, basename);
  return NULL;
}

/**
 * Set executable permissions on a file
 * @return: 0 on success, else errno
 */
int sxupdate_set_execute_permission(const char *path) {
#if !defined(_WIN32) && !defined(WIN32) && !defined(WIN)
  if(chmod(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) != 0) {
    sxupdate_printerr("Could not set execute permissions: %s", path);
    return errno;
  }
#else
  (void)(path);
#endif
  return 0;
}
