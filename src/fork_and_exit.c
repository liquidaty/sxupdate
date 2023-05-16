#include <stdio.h>
#include <stdlib.h>
#include "internal.h"

#ifdef _WIN32
# include <windows.h>
# include <tchar.h>
#else
# include <unistd.h>
#endif

/***

 *** Usage example ***


 **/

#ifdef _WIN32

static WCHAR *utf8ToWide(const char *s_utf8) {
  int s_len = strlen(s_utf8);

  // Get the required size of the new buffer
  int wideCharSize = MultiByteToWideChar(CP_UTF8, 0, s_utf8, s_len, NULL, 0);
  if(wideCharSize == 0) {
    fprintf(stderr, "Error in MultiByteToWideChar!\n");
    return NULL;
  }

  // Allocate the new wide char buffer
  WCHAR* result = (WCHAR*)malloc(sizeof(WCHAR) * (wideCharSize + 1));
  if(result == NULL) {
    fprintf(stderr, "Out of memory!\n");
    return NULL;
  }

  // Convert the string
  int rc = MultiByteToWideChar(CP_UTF8, 0, s_utf8, s_len, result, wideCharSize);
  if(rc == 0) {
    fprintf(stderr, "Error in MultiByteToWideChar!\n");
    free(result);
    return NULL;
  }

  // Null-terminate the wide string
  result[wideCharSize] = '\0';
  return result;
}

#endif

/****
 * fork_and_exit(): return 0 on success
 *
 * @param executable_path: e.g. _T("C:\\Path\\To\\Your\\Executable.exe") or "/path/to/your/program"
 ****/
int fork_and_exit(char *executable_path, struct sxupdate_string_list *args,
                  unsigned char verbosity) {
  if(verbosity) {
    fprintf(stderr, "Executing: %s\n", executable_path);
    if(args) {
      for(struct sxupdate_string_list *arg = args; arg; arg = arg->next)
        fprintf(stderr, "  %s\n", arg->value);
    }
    fprintf(stderr, "\n");
  }

#ifdef _WIN32
  char *cmd = executable_path;
  if(args) {
    size_t len = strlen(executable_path);
    for(struct sxupdate_string_list *arg = args; arg; arg = arg->next)
      len += 1 + strlen(arg->value);
    len += 2;

    cmd = calloc(1, len);
    strcat(cmd, executable_path);
    for(struct sxupdate_string_list *arg = args; arg; arg = arg->next) {
      strcat(cmd, " ");
      strcat(cmd, arg->value);
    }
  }
  if(verbosity)
    fprintf(stderr, "Final cmd: %s\n", cmd);

  WCHAR *path_w = utf8ToWide(cmd);
  if(cmd != executable_path)
    free(cmd);
  if(!path_w)
    return 1;

  STARTUPINFOW si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  int rc = CreateProcessW(NULL,   // No module name (use command line)
                          path_w, // e.g. _T("C:\\Path\\To\\Your\\Executable.exe")
                          NULL,           // Process handle not inheritable
                          NULL,           // Thread handle not inheritable
                          FALSE,          // Set handle inheritance to FALSE
                          0,              // No creation flags
                          NULL,           // Use parent's environment block
                          NULL,           // Use parent's starting directory
                          &si,            // Pointer to STARTUPINFO structure
                          &pi);           // Pointer to PROCESS_INFORMATION structure
  free(path_w);
  if(!rc) {
    DWORD errorMessageID = GetLastError();
    LPSTR messageBuffer = NULL;
    size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                 NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);
    // If FormatMessage returns a size greater than zero, it was able to format the error message
    if(size)
      printf("CreateProcess failed with error x: %s\n", messageBuffer);
    else
      printf("CreateProcess failed with error: %lu\n", errorMessageID);
    LocalFree(messageBuffer);
    return 1;
  }
#else
  pid_t pid = fork();
  if(pid < 0) {
    fprintf(stderr, "Fork Failed");
    return 1;
  }

  if(pid == 0) {
    size_t argc = 1;
    for(struct sxupdate_string_list *arg = args; arg; arg = arg->next)
      argc++;

    char **argv = calloc(argc + 1, sizeof(*argv));
    if(!argv) {
      fprintf(stderr, "Out of memory!");
      return 1;
    }

    argv[0] = executable_path;
    int i = 1;
    for(struct sxupdate_string_list *arg = args; arg; arg = arg->next)
      argv[i++] = arg->value;
    argv[i++] = NULL;

    execv(argv[0], argv);
    // if execv succeeds, we won't get here and the OS will free argv
    // otherwise, execv failed and we need to free argv ourselves
    fprintf(stderr, "Execv Failed");
    free(argv);
    return 1;
  }
#endif

  return 0;
}
