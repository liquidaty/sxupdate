#ifndef SXUPDATE_H

typedef struct sxupdate_data *sxupdater;

struct sxupdate_version {
  struct {
    int major;
    int minor;
    int patch;
  } version;

  struct {
    const char *title;
    const char *link;
    const char *version;
    const char *description;
    const char *pubDate;
    struct {
      const char *url;
      size_t length;
      const char *type;
      const char *signature;
    } enclosure;
  } item;
};

/***
 * Provide a callback to determine if there is a newer version and if so,
 * whether to continue with the update
 *
 * The callback should manage the process of:
 * - checking whether the version is in fact newer
 * - confirming that the user wants to proceed with the update
 *
 * The callback should then call next with do_update set to non-zero to proceed with the update, or zero not to proceed with any update
 **/
void sxupdate_install_version_callback(sxupdater *,
                                       void (*install_newer_version)(const struct sxupdate_version *version, void (*next)(sxupdater *, int do_update)));

/***
 * Parse the JSON file containing the latest version info
 **/
enum yajl_status sxupdate_parse(sxupdater *, const char *data, size_t len);



#ifndef SXUPDATE_H
