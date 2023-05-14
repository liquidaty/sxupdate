#ifndef SXUPDATE_API_H
#define SXUPDATE_API_H

#define SXUPDATE_API

#include <ctype.h>

typedef struct sxupdate_data *sxupdate_t;

enum sxupdate_status {
  sxupdate_status_ok = 0,
  sxupdate_status_error,
  sxupdate_status_memory,
  sxupdate_status_bad_url,
  sxupdate_status_invalid, /* invalid option value */
  sxupdate_status_parse    /* parse error */
};

enum sxupdate_action {
  sxupdate_action_proceed = 100,
  sxupdate_action_abort = 200
};

struct sxupdate_semantic_version { /* see https://semver.org */
  int major;
  int minor;
  int patch;
  char *prerelease;
  char *meta;
};

struct sxupdate_version { // structure for an appcast item
  char *title;
  char *link;
  char *description;
  char *pubDate;

  struct sxupdate_semantic_version version;

  struct {
    char *url;
    size_t length;
    char *type;
    char *signature;
    char *filename; /* name of downloaded file e.g. 'myapp_installer.exe' */
  } enclosure;
};


/***
 * Get a new sxupdate handle
 **/
sxupdate_t sxupdate_new();

/***
 * Delete a handle that was created with sxupdate_new()
 */
void sxupdate_delete(sxupdate_t handle);

/***
 * Set verbosity level (for debugging)
 *
 * @param verbosity: number between 0 (normal) and 5 (maximum). Any value > 5 is treated as 5
 */
void sxupdate_set_verbosity(sxupdate_t handle, unsigned char verbosity);

/***
 * Add an argument that will be passed to the installer when it is invoked
 * In windows environment, the argument will be appended to the command string,
 * and the caller must handle any necessary escaping (e.g. quotation marks)
 *
 * @param dir: the argument to add e.g. /D=C:\Program Files\NSIS
 */
enum sxupdate_status sxupdate_add_installer_arg(sxupdate_t handle, const char *value);

/***
 * Set the callback that will inform sxupdate what this build's version is, so that
 * it can compare to the version metadata it fetches
 */
void sxupdate_set_current_version(sxupdate_t handle,
                                  struct sxupdate_semantic_version (*cb)());

/***
 * Set the callback that will be called in the event an updated version is available
 * Use this if your callback will execute synchronously
 */
void sxupdate_on_update_available(sxupdate_t handle,
                                  enum sxupdate_action (*cb)(const struct sxupdate_version *version));

/***
 * Set the url used to fetch the version info. The url value can be transient, and must
 * start with http:// or file://
 */
enum sxupdate_status sxupdate_set_url(sxupdate_t handle, const char *url);


/***
 * Set a custom header to send with the fetch request. The header name and value can be transient
 */
enum sxupdate_status sxupdate_add_header(sxupdate_t handle, const char *header_name, const char *header_value);

/***
 * Execute the update
 *
 * @return 0 on success, else non-zero.
 **/
enum sxupdate_status sxupdate_execute(sxupdate_t handle);

/***
 * Retrieve the last error message, if any [NOT YET IMPLEMENTED]
 */
const char *sxupdate_err_msg(sxupdate_t handle);

#ifndef NO_SIGNATURE
#include <openssl/rsa.h>

/**
 * Set public key. This function must be called at least once before sxupdate_execute()
 * can be run
 */
void sxupdate_set_public_key(sxupdate_t handle, RSA *key);

enum sxupdate_status sxupdate_set_public_key_from_file(sxupdate_t handle, const char *filepath);

#endif


#endif // SXUPDATE_API_H
