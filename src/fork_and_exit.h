#ifndef SXUPDATE_FORK_AND_EXIT_H
#define SXUPDATE_FORK_AND_EXIT_H

int fork_and_exit(char *executable_path, struct sxupdate_string_list *args,
                  unsigned char verbosity);

#endif
