#include <stdio.h>
#include <string.h>
#include <sxupdate/api.h>

static char *fgets_no_trailing_white(char * restrict str, int size, FILE * restrict stream) {
  char *s = fgets(str, size, stream);
  while(s && *s && strchr(" \n\r\t\f", s[strlen(s)-1]))
    s[strlen(s)-1] = '\0';
  return s && *s ? s : NULL;
}

static enum sxupdate_action ask_to_proceed(const struct sxupdate_version *version) {
  fprintf(stderr, "A newer version of this software is available. Download and update now (Y)?\n");
  char answer[200];
  if(fgets_no_trailing_white(answer, sizeof(answer), stdin) && *answer && strchr("Yy", *answer))
    return sxupdate_action_proceed;
  return sxupdate_action_abort;
}

struct sxupdate_semantic_version get_version() {
  struct sxupdate_semantic_version v = {};
  v.major = 1;
  v.minor = 1;
  v.patch = 1;
  v.prerelease = "alpha";
  return v;
}

int main(int argc, const char *argv[]) {
  char url[1024];
  fprintf(stderr, "Enter URL to fetch appcast.json file from (should begin with 'https://' or 'file://'\n e.g. https://raw.githubusercontent.com/liquidaty/sxupdate/main/test/example1/appcast.json):\n");
  if(!fgets_no_trailing_white(url, sizeof(url), stdin))
    return 1;

  char have_file = 0;
  size_t len = strlen(url);
  if(len < 9 || memcmp(url, "https://", 8)) {
    if(len > 8 && !memcmp(url, "file://", 7))
      have_file = 1;
    else {
      fprintf(stderr, "Invalid URL: %s\n", url);
      return 1;
    }
  }

  char have_custom_header = 0;
  char header_name[128];
  char header_value[512];
  if(!have_file) {
    fprintf(stderr, "Enter custom header name (e.g. 'Authorization') to send with request, if any\n");
    if(fgets_no_trailing_white(header_name, sizeof(header_name), stdin)) {

      fprintf(stderr, "Enter custom header value (e.g. 'token xxxxxyyyyy')\n");
      if(!fgets_no_trailing_white(header_value, sizeof(header_value), stdin))
        *header_value = '\0';

      if(!*header_value) {
        fprintf(stderr, "No header value provided!\n");
        return 1;
      }
      have_custom_header = 1;
    }
  }

  sxupdate_t sxu = sxupdate_new();
  if(sxu) {
    sxupdate_set_current_version(sxu, get_version);
    sxupdate_on_update_available(sxu, ask_to_proceed);
    sxupdate_set_url(sxu, url);
    if(have_custom_header)
      sxupdate_add_header(sxu, header_name, header_value);
    if(sxupdate_execute(sxu))
      fprintf(stderr, "Error: %s\n", sxupdate_err_msg(sxu));
    sxupdate_delete(sxu);
  }
}
