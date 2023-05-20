#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#ifndef NO_SIGNATURE
#include <openssl/rsa.h>
#endif

#include "../include/api.h"
#include "internal.h"
#include "file.h"
#include "fork_and_exit.h"
#include "parse.h"
#include "version.h"
#include "verify.h"
#include "log.h"

/***
 * Get a new sxupdate handle
 **/
SXUPDATE_API sxupdate_t sxupdate_new() {
  sxupdate_t h = calloc(1, sizeof(struct sxupdate_data));
  if(h)
    h->parser.stat = yajl_status_ok;
  return h;
}

/***
 * Set the callback that will inform sxupdate what this build's version is, so that
 * it can compare to the version metadata it fetches
 */
SXUPDATE_API void sxupdate_set_current_version(sxupdate_t handle,
                                               struct sxupdate_semantic_version (*cb)()) {
  handle->get_current_version = cb;
}

/***
 * Set verbosity level (for debugging)
 *
 * @param verbosity: number between 0 (normal) and 5 (maximum). Any value > 5 is treated as 5
 */
SXUPDATE_API void sxupdate_set_verbosity(sxupdate_t handle, unsigned char verbosity) {
  handle->verbosity = (verbosity > 5 ? 5 : verbosity);
}

/***
 * Add an argument that will be passed to the installer when it is invoked
 * In windows environment, the argument will be appended to the command string,
 * and the caller must handle any necessary escaping (e.g. quotation marks)
 *
 * @param dir: the argument to add e.g. /D=C:\Program Files\NSIS
 */
enum sxupdate_status sxupdate_add_installer_arg(sxupdate_t handle, const char *val) {
  struct sxupdate_string_list *arg = calloc(1, sizeof(*arg));
  char *value = strdup(val);
  if(!(arg && value)) {
    free(arg);
    free(value);
    return sxupdate_status_memory;
  }
  arg->value = value;
  if(handle->installer_args == NULL)
    handle->installer_args_next = &handle->installer_args;
  *handle->installer_args_next = arg;
  handle->installer_args_next = &arg->next;

  return sxupdate_status_ok;
}

static void sxupdate_free(sxupdate_t handle) {
  if(handle->http_headers)
    curl_slist_free_all(handle->http_headers);

  if(handle->public_key)
    RSA_free(handle->public_key);

  sxupdate_version_free(&handle->latest_version);

  for(struct sxupdate_string_list *next, *arg = handle->installer_args; arg; arg = next) {
    next = arg->next;
    free(arg->value);
    free(arg);
  }
  free(handle->latest_version_internal.signature);
  free(handle->url);
  yajl_helper_parse_state_free(&handle->parser.st);
}


#ifndef NO_SIGNATURE
/**
 * Set public key. This function must be called at least once before sxupdate_execute()
 * can be run
 */
SXUPDATE_API void sxupdate_set_public_key(sxupdate_t handle, RSA *key) {
  handle->no_public_key = 0;
  if(handle->public_key)
    RSA_free(handle->public_key);
  if(!key) {
    sxupdate_printerr("Warning! signature verification disabled. Not secure!!!!!");
    handle->no_public_key = 1;
    handle->public_key = NULL;
  } else
    handle->public_key = key;
}

SXUPDATE_API enum sxupdate_status sxupdate_set_public_key_from_file(sxupdate_t handle, const char *filepath) {
  if(handle->public_key)
    RSA_free(handle->public_key);
  handle->no_public_key = 0;
  if(handle->verbosity)
    sxupdate_verbose("loading public key from file %s", filepath);
  handle->public_key = sxupdate_public_key_from_pem_file(filepath);
  if(!handle->public_key)
    return sxupdate_status_error;
  return sxupdate_status_ok;
}

#endif

/***
 * Delete a handle that was created with sxupdate_new()
 */
SXUPDATE_API void sxupdate_delete(sxupdate_t handle) {
  sxupdate_free(handle);
  free(handle);
}

/***
 * Set the callback that will be called in the event an updated version is available
 */
SXUPDATE_API void sxupdate_set_interaction_handler(sxupdate_t handle,
                                                   sxupdate_interaction_handler handler) {
  handle->interaction_handler = handler;
}


/***
 * Set the url used to fetch the version info. The url value can be transient, and must
 * start with http:// or file://
 */
SXUPDATE_API enum sxupdate_status sxupdate_set_url(sxupdate_t handle, const char *url) {
  if(!url) {
    free(handle->url);
    handle->url = NULL;
    return sxupdate_status_bad_url;
  }

  if(sxupdate_url_is_https(url))
    handle->url_is_file = 0;
  else if(sxupdate_url_is_file(url))
    handle->url_is_file = 1;
  else
    return sxupdate_status_bad_url;

  free(handle->url);
  handle->url = strdup(url);
  return sxupdate_status_ok;
}

/***
 * Set a custom header to send with the fetch request. The header name and value can be transient
 */
SXUPDATE_API enum sxupdate_status sxupdate_add_header(sxupdate_t handle, const char *header_name, const char *header_value) {
  if(!(header_name && header_value && *header_name && *header_value))
    return sxupdate_status_invalid;

  size_t len = strlen(header_name) + strlen(header_value) + 10;
  char *s = calloc(1, len);
  if(!s)
    return sxupdate_status_memory;
  snprintf(s, len, "%s: %s", header_name, header_value);
  if(handle->verbosity)
    sxupdate_verbose("Adding header %s", s);
  handle->http_headers = curl_slist_append(handle->http_headers, s);
  free(s);
  return sxupdate_status_ok;
}

static enum sxupdate_status sxupdate_ready(sxupdate_t handle) {
  if(!handle->get_current_version) {
    sxupdate_printerr("get_current_version callback not set");
    return sxupdate_status_error;
  }
  if(!handle->url || !handle->interaction_handler) {
    sxupdate_printerr("url or on_update_available callback not set");
    return sxupdate_status_error;
  }
  if(handle->url_is_file && handle->http_headers) {
    sxupdate_printerr("custom headers cannot be used with file:// url");
    return sxupdate_status_error;
  }

  if(!handle->public_key && !handle->no_public_key) {
    sxupdate_printerr("no public key set-- call sxupdate_set_public_key() before sxupdate_execute()");
    return sxupdate_status_error;
  }

  return sxupdate_status_ok;
}

static size_t sxupdate_curl_progress_callback(void *h,
                                              double dltotal,
                                              double dlnow,
                                              double ultotal,
                                              double ulnow) {
  (void)(dltotal);
  (void)(dlnow);
  (void)(ultotal);
  (void)(ulnow);
  sxupdate_t handle = h;
  if(handle->parser.stat != yajl_status_ok)
    return 1; // abort
   return 0; /* all is good */
}

static size_t sxupdate_parse_chunk(char *ptr, size_t size, size_t nmemb, void *h) {
  sxupdate_t handle = h;
  size_t len = size * nmemb;
  if(handle->parser.stat == yajl_status_ok)
    sxupdate_parse(handle, ptr, len);
  return len;
}

static enum sxupdate_status sxupdate_after_parse(sxupdate_t handle, enum sxupdate_status stat,
                                                 void (*next)(sxupdate_t, enum sxupdate_status)
                                                 ) {
  // finish parsing
  if(stat == sxupdate_status_ok) {
    if(handle->parser.scanned_bytes == 0) {
      handle->err_msg = "Unable to connect. Please check your credentials and try again";
      stat = sxupdate_status_error;
    } else
      stat = sxupdate_parse_finish(handle);
  }
  next(handle, stat);
  return stat;
}

/***
 * Fetch and parse the metadata from file or network using curl
 */
static enum sxupdate_status sxupdate_fetch_from_curl(sxupdate_t handle,
                                                     struct curl_slist *http_headers,
                                                     void (*next)(sxupdate_t, enum sxupdate_status)
                                                     ) {
  enum sxupdate_status stat = sxupdate_status_ok;
  CURL *curl = curl_easy_init();
  if(!curl)
    stat = sxupdate_status_memory;
  else {
    curl_easy_setopt(curl, CURLOPT_URL, handle->url);

    // set custom headers
    if(http_headers && !sxupdate_url_is_file(handle->url))
      curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, handle);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, sxupdate_curl_progress_callback);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(curl, CURLOPT_WRITEDATA, handle);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sxupdate_parse_chunk);

#ifdef _WIN32
    // if(no_verify)
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif

    // execute
    if(handle->verbosity)
      sxupdate_verbose("Fetching version info from %s", handle->url);

    CURLcode res = curl_easy_perform(curl);

    if((sxupdate_url_is_file(handle->url)))
      handle->http_code = 200;
    else
      curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &handle->http_code);

    if(res != CURLE_OK) {
      sxupdate_printerr("Error connecting to %s:\n  %s", handle->url, curl_easy_strerror(res));
      stat = sxupdate_status_error;
    } else if(!(handle->http_code >= 200 && handle->http_code < 300))
      stat = sxupdate_status_error;

    curl_easy_cleanup(curl);

    stat = sxupdate_after_parse(handle, stat, next);
  }

  return stat;
}

/***
 * Fetch and parse the metadata from network or file
 *
 * This implementation is synchronous, but, we use a callback because future
 * implementations in other contexts (e.g. web assembly) might be async.
 */
static
enum sxupdate_status sxupdate_fetch_and_parse(sxupdate_t handle,
                                              struct curl_slist *http_headers,
                                              void (*next)(sxupdate_t, enum sxupdate_status)
                                              ) {
  enum sxupdate_status stat = sxupdate_status_error;
  if(handle->url
     && (stat = sxupdate_parse_init(handle)) == sxupdate_status_ok)
    stat = sxupdate_fetch_from_curl(handle, http_headers, next);
  return stat;
}


static char *url_merge(const char *parent_url, const char *relative_url) {
  size_t parent_bytes_to_keep;
  if(strncmp(parent_url, SXUPDATE_HTTPS_PREFIX, strlen(SXUPDATE_HTTPS_PREFIX))) {
    char *first_slash = strchr(parent_url + strlen(SXUPDATE_HTTPS_PREFIX), '/');
    if(!first_slash) {
      sxupdate_printerr("url_merge: unexpected error 1");
      return NULL;
    }
    parent_bytes_to_keep = first_slash - parent_url + 1;
  } else if(strncmp(parent_url, SXUPDATE_FILE_PREFIX, strlen(SXUPDATE_FILE_PREFIX)))
    parent_bytes_to_keep = strlen(SXUPDATE_FILE_PREFIX);
  else {
    sxupdate_printerr("url_merge: unexpected error 2");
    return NULL;
  }

  if(!strncmp(relative_url, "./", 2))
    relative_url += 2;

  if(*relative_url != '/') {
    // relative path. extend parent_bytes_to_keep to the last slash
    char *last_slash = strrchr(parent_url + parent_bytes_to_keep, '/');
    if(last_slash && last_slash > parent_url + parent_bytes_to_keep)
      parent_bytes_to_keep = last_slash - parent_url + 1;
  } else
    relative_url++;
  size_t len = parent_bytes_to_keep + strlen(relative_url) + 3;
  char *merged_url = malloc(len);
  snprintf(merged_url, len, "%.*s%s", (int)parent_bytes_to_keep, parent_url, relative_url);
  return merged_url;
}

/***
 * Download the installer file and save the file path to save_path_p
 *
 * @return sxupdate_status_ok on success
 */
static enum sxupdate_status sxupdate_download(sxupdate_t handle,
                                              char **save_path_p) {
  const char *parent_url = handle->url;
  struct sxupdate_version *version = &handle->latest_version;
  struct curl_slist *http_headers = handle->http_headers;
  unsigned char verbosity = handle->verbosity;

  *save_path_p = NULL;
  char *resolved_url = NULL;
  if(sxupdate_is_relative_filename(version->enclosure.url)) {
    if(verbosity > 1)
      sxupdate_verbose("Merging urls: %s + %s", parent_url, version->enclosure.url);
    resolved_url = url_merge(parent_url, version->enclosure.url);
    if(!resolved_url) {
      sxupdate_printerr("Unable to merge urls: %s + %s", parent_url, version->enclosure.url);
      return sxupdate_status_error;
    }
  } else
    resolved_url = version->enclosure.url;

  enum sxupdate_status stat = sxupdate_status_error;
  char *save_path = sxupdate_get_installer_download_path(version->enclosure.filename);

  // download to temp file
  if(!save_path)
    stat = sxupdate_status_memory;
  else {
    if(verbosity)
      sxupdate_verbose("Downloading to %s from %s", save_path, resolved_url);
    FILE *f = fopen(save_path, "wb");
    if(!f)
      perror(save_path);
    else {
      // initialize curl
      CURL *curl = curl_easy_init();
      if(!curl)
        stat = sxupdate_status_memory;
      else {
        curl_easy_setopt(curl, CURLOPT_URL, resolved_url);

        // set custom headers
        if(http_headers)
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

        // to do: add option for custom progress reporting

        // set write to temp file
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

#ifdef _WIN32
        // if(no_verify)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
#endif

        // connect and download
        CURLcode res = curl_easy_perform(curl);
        if((sxupdate_url_is_file(resolved_url)))
          handle->http_code = 200;
        else
          curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &handle->http_code);
        stat = sxupdate_status_error;
        if(res != CURLE_OK)
          sxupdate_printerr("Error connecting to %s:\n  %s", resolved_url, curl_easy_strerror(res));
        else if(handle->http_code >= 200 && handle->http_code < 300)
          stat = sxupdate_status_ok;
        if(verbosity > 2)
          sxupdate_verbose("cleaning up curl call");
        curl_easy_cleanup(curl);
      }
      fclose(f);
    }
  }

  if(stat != sxupdate_status_ok)
    free(save_path);
  else
    *save_path_p = save_path;

  if(resolved_url != version->enclosure.url)
    free(resolved_url);

  return stat;
}

static void sxupdate_resume(sxupdate_t handle, enum sxupdate_action action) {
  enum sxupdate_status stat = sxupdate_status_ok;
  if(action == sxupdate_action_proceed && handle->step == sxupdate_step_have_newer_version) {
    char *downloaded_file_path;
    stat = sxupdate_download(handle, &downloaded_file_path);
    if(stat == sxupdate_status_ok &&
       // ensure saved_path has executable permissions
       sxupdate_set_execute_permission(downloaded_file_path))
      stat = sxupdate_status_error;
    if(stat == sxupdate_status_ok) {
      // TO DO: check download file size

      // check signature
      stat = sxupdate_verify_signature(handle, downloaded_file_path);
      if(stat == sxupdate_status_ok) {
        if(fork_and_exit(downloaded_file_path, handle->installer_args, handle->verbosity))
          stat = sxupdate_status_error;
      }
    }
    free(downloaded_file_path);
  }
}

static void sxupdate_after_fetch_and_parse(sxupdate_t handle, enum sxupdate_status stat) {
  if(stat == sxupdate_status_ok) {
    handle->http_code = 0;

    // check if this version is newer
    if(sxupdate_version_cmp(handle->latest_version.version, handle->get_current_version(), handle->verbosity) > 0)
      handle->step = sxupdate_step_have_newer_version;
    else
      handle->step = sxupdate_step_already_up_to_date;

    // execute callback and proceed if it returns sxupdate_action_do_update
    handle->interaction_handler(handle, handle->step, sxupdate_resume);
  }
}

/***
 * Get a pointer to the latest version, as defined by the fetched metadata
 * For use within your interaction handler, for example to display a message
 * to the user containing details about the new available version
 */
const struct sxupdate_version *sxupdate_get_version(sxupdate_t handle) {
  return &handle->latest_version;
}

/***
 * Execute the update
 *
 * @return 0 on success, else non-zero.
 **/
SXUPDATE_API enum sxupdate_status sxupdate_execute(sxupdate_t handle) {
  enum sxupdate_status stat;

  // make sure our handle is prepared
  if((stat = sxupdate_ready(handle)) == sxupdate_status_ok) {

    // fetch the update metadata
    stat = sxupdate_fetch_and_parse(handle, handle->http_headers, sxupdate_after_fetch_and_parse);
  }
  return stat;
}

/***
 * Retrieve the last error message. Caller must free
 */
SXUPDATE_API char *sxupdate_err_msg(sxupdate_t handle) {
  if(handle->err_msg)
    return strdup(handle->err_msg);
  if(handle->http_code > 0 && (handle->http_code >= 400 || handle->http_code < 200)) {
    switch(handle->http_code) {
    case 401:
      return strdup("Unauthorized. Please check your Hub credentials and try again");
    default:
      {
        char *s = malloc(100);
        snprintf(s, 100, "Http response code %li", handle->http_code);
        return s;
      }
    }
  }
  return strdup("Unexpected error");
}
