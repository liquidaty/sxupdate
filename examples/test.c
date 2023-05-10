#include <stdio.h>

#include <sxupdate/api.h>

int main(int argc, const char *argv[]) {
  printf("hi");
  static void install_newer_version(struct sxupdate_version *version, void (*proceed_with_update)(sxupdater *));

  sxupdater *sxu = sxupdate_init();
  if(sxu) {
    sxupdate_install_version_callback(sxu, install_newer_version);
    sxupdate_set_url(sxu, my_fetch, my_fetch_ctx);
    sxupdate_execute(sxu);
  }
}



static void install_newer_version(const struct sxupdate_version *version, void (*proceed_with_update)(sxupdater *)) {
  ...
  if(proceed) {
  proceed_with_update
  }
 }
