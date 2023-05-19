#ifndef SXUPDATE_FILE_H
#define SXUPDATE_FILE_H
/**
 * Get a file name to download the installation executable to. The returned value,
 * if any, will have been allocated on the heap, and the caller should free it using `free()`
 *
 * @param prefix string with which the resulting file name will be prefixed
 */
char *sxupdate_get_installer_download_path(const char *basename);


/**
 * Set executable permissions on a file
 * @return: 0 on success
 */
int sxupdate_set_execute_permission(const char *path);

#endif
