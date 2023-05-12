#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "../include/api.h"
#include "internal.h"
#include "file.h"
#include "fork_and_exit.h"
#include "parse.h"
#include "version.h"

/***
 * Get a new sxupdate handle
 **/
SXUPDATE_API sxupdate_t sxupdate_new() {
  return calloc(1, sizeof(struct sxupdate_data));
}

/***
 * Set the callback that will inform sxupdate what this build's version is, so that
 * it can compare to the version metadata it fetches
 */
SXUPDATE_API void sxupdate_set_current_version(sxupdate_t handle,
                                               struct sxupdate_semantic_version (*cb)()) {
  handle->get_current_version = cb;
}

static void sxupdate_free(sxupdate_t handle) {
  if(handle->http_headers)
    curl_slist_free_all(handle->http_headers);
  handle->http_headers = NULL;

  free(handle->url);
  handle->url = NULL;
  yajl_helper_parse_state_free(&handle->parser.st);
}

/***
 * Delete a handle that was created with sxupdate_new()
 */
SXUPDATE_API void sxupdate_delete(sxupdate_t handle) {
  sxupdate_free(handle);
  // if(!handle->no_heap)
  free(handle);
}

/***
 * Set the callback that will be called in the event an updated version is available
 * Use this if your callback will execute synchronously
 */
SXUPDATE_API void sxupdate_on_update_available(sxupdate_t handle,
                                               enum sxupdate_action (*cb)(const struct sxupdate_version *version)
                                               ) {
  handle->sync_cb = cb;
}

/***
 * TO DO:
 * Set the async callback that will be called in the event an updated version is available
 * Use this if your callback will execute asynchronously
 * To proceed, the callback should call proceed(handle)
 * To ignore, the callback should call ignore(ignore_ctx)
void sxupdate_on_update_available_async(sxupdate_t handle,
                                        void (*cb)(const struct sxupdate_version *version,
                                                   void (*proceed)(sxupdate_t *),
                                                   void (*ignore)(void *ctx),
                                                   void *ignore_ctx)
                                        ) {
  fprintf(stderr, "sxupdate_on_update_available_async not yet implemented!\n");
  // handle->async_cb = cb;
}
 **/


/***
 * Set the url used to fetch the version info. The url value can be transient, and must
 * start with http:// or file://
 */
SXUPDATE_API enum sxupdate_status sxupdate_set_url(sxupdate_t handle, const char *url) {
  if(!url) {
    free(handle->url);
    handle->url = NULL;
  } else {
    switch(sxupdate_url_ok(url)) {
    case 0:
      return sxupdate_status_bad_url;
    case 1:
      handle->url_is_file = 0;
      break;
    case 2:
      handle->url_is_file = 1;
      break;
    }
    free(handle->url);
    handle->url = strdup(url);
  }
  return sxupdate_status_ok;
}

/***
 * Set a custom header to send with the fetch request. The header name and value can be transient
 */
SXUPDATE_API enum sxupdate_status sxupdate_add_header(sxupdate_t handle, const char *header_name, const char *header_value) {
  if(!(header_name && header_value && *header_name && *header_value))
    return sxupdate_status_invalid;

  char *s;
  asprintf(&s, "%s: %s", header_name, header_value);
  if(!s)
    return sxupdate_status_memory;
  handle->http_headers = curl_slist_append(handle->http_headers, s);
  free(s);
  return sxupdate_status_ok;
}

static enum sxupdate_status sxupdate_ready(sxupdate_t handle) {
  if(!handle->get_current_version) {
    fprintf(stderr, "get_current_version callback not set\n");
    return sxupdate_status_error;
  }
  if(!handle->url || !handle->sync_cb) {
    fprintf(stderr, "url or on_update_available callback not set\n");
    return sxupdate_status_error;
  }
  if(handle->url_is_file && handle->http_headers) {
    fprintf(stderr, "custom headers cannot be used with file:// url\n");
    return sxupdate_status_error;
  }

  return sxupdate_status_ok;
}

static size_t sxupdate_parse_chunk(char *ptr, size_t size, size_t nmemb, void *handle) {
  size_t len = size * nmemb;
  if(sxupdate_parse(handle, ptr, len) == sxupdate_status_ok)
    return len;
  return 0;
}

/***
 * Fetch and parse the metadata
 *
 * @param contentp: on success, will be assigned the location of the fetched content
 */
static enum sxupdate_status sxupdate_fetch_and_parse(sxupdate_t handle, struct curl_slist *http_headers) {
  enum sxupdate_status stat = sxupdate_status_error;
  if(handle->url
     && (stat = sxupdate_parse_init(handle)) == sxupdate_status_ok) { // initialize parser

    // do the parse
    if(handle->url_is_file) {
      const char *filename = handle->url + strlen("file://");
      FILE *f = fopen(filename, "rb");
      if(f) {
        char buff[1024];
        size_t len;
        while(stat == sxupdate_status_ok && (len = fread(buff, 1, sizeof(buff), f)) > 0)
          stat = sxupdate_parse(handle, buff, len);
        fclose(f);
      }
    } else {
      CURL *curl = curl_easy_init();
      if(!curl)
        stat = sxupdate_status_memory;
      else {
        curl_easy_setopt(curl, CURLOPT_URL, handle->url);

        // set custom headers
        if(http_headers)
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

        curl_easy_setopt(curl, CURLOPT_WRITEDATA, handle);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, sxupdate_parse_chunk);

        // if(no_verify)
        //  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        //  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

        // execute
        CURLcode res = curl_easy_perform(curl);
        if(res != CURLE_OK) {
          fprintf(stderr, "Error connecting to %s:\n  %s\n", handle->url, curl_easy_strerror(res));
          stat = sxupdate_status_error;
        }
      }
    }

    // finish parsing
    if(stat == sxupdate_status_ok)
      stat = sxupdate_parse_finish(handle);
  }
  return stat;
}

/***
 * Download the installer file and save the file path to save_path_p
 *
 * @return sxupdate_status_ok on success
 */
static enum sxupdate_status sxupdate_download(struct sxupdate_version *version,
                                              struct curl_slist *http_headers,
                                              char **save_path_p) {
  if(sxupdate_url_ok(version->enclosure.url) == 2) {
    *save_path_p = strdup(version->enclosure.url + strlen("file://"));
    return sxupdate_status_ok;
  }

  enum sxupdate_status stat = sxupdate_status_error;
  char *save_path = sxupdate_get_installer_download_path(version->enclosure.filename);

  // download to temp file
  if(!save_path)
    stat = sxupdate_status_memory;
  else {
    FILE *f = fopen(save_path, "wb");
    if(!f)
      perror(save_path);
    else {
      // initialize curl
      CURL *curl = curl_easy_init();
      if(!curl)
        stat = sxupdate_status_memory;
      else {
        curl_easy_setopt(curl, CURLOPT_URL, version->enclosure.url);

        // set custom headers
        if(http_headers)
          curl_easy_setopt(curl, CURLOPT_HTTPHEADER, http_headers);

        // set write to temp file
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, f);

        // if(no_verify)
        //  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        //  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);

        // connect and download
        CURLcode res = curl_easy_perform(curl);

        if(res != CURLE_OK)
          fprintf(stderr, "Error connecting to %s:\n  %s\n", version->enclosure.url, curl_easy_strerror(res));
        else {
          // TO DO: check download file size, check signature

          // ensure saved_path has executable permissions
          if(!sxupdate_set_execute_permission(save_path))
            stat = sxupdate_status_ok;
        }
      }
    }
  }

  if(stat != sxupdate_status_ok) {
    free(save_path);
    *save_path_p = NULL;
  } else
    *save_path_p = save_path;
  return stat;
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
    if((stat = sxupdate_fetch_and_parse(handle, handle->http_headers)) == sxupdate_status_ok) {

      // check if this version is newer
      if(sxupdate_version_cmp(handle->latest_version.version, handle->get_current_version()) > 0) {

        // execute callback and proceed if it returns sxupdate_action_do_update
        if(handle->sync_cb(&handle->latest_version) == sxupdate_action_proceed) {
          char *downloaded_file_path;
          stat = sxupdate_download(&handle->latest_version, handle->http_headers, &downloaded_file_path);
          if(stat == sxupdate_status_ok) {
            if(fork_and_exit(downloaded_file_path))
              stat = sxupdate_status_error;
          }
          free(downloaded_file_path);
        }
      }
    }
  }
  return stat;
}

/***
 * Retrieve the last error message, if any [NOT YET IMPLEMENTED]
 */
SXUPDATE_API const char *sxupdate_err_msg(sxupdate_t handle) {
  (void)(handle);
  return "to do: sxupdate_err_msg()";
}
