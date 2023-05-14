#include <stdio.h>
#include <stdlib.h>

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
int fork_and_exit(char *executable_path, unsigned char verbosity) {
  if(verbosity)
    fprintf(stderr, "Fork and execute: %s\n", executable_path);
#ifdef _WIN32
  WCHAR *path_w = utf8ToWide(executable_path);
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
    char * argv[] = {executable_path, NULL}; // "/path/to/your/program", NULL};
    execv(argv[0], argv);
    fprintf(stderr, "Execv Failed");
    return 1;
  }
#endif

  return 0;
}
