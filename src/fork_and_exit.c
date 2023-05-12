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

/****
 * fork_and_exit(): return 0 on success
 *
 * @param executable_path: e.g. _T("C:\\Path\\To\\Your\\Executable.exe") or "/path/to/your/program"
 ****/
int fork_and_exit(char *executable_path) {
#ifdef _WIN32
  STARTUPINFO si;
  PROCESS_INFORMATION pi;

  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  ZeroMemory(&pi, sizeof(pi));
  if(!CreateProcess(NULL,   // No module name (use command line)
                    executable_path, // e.g. _T("C:\\Path\\To\\Your\\Executable.exe")
                    NULL,           // Process handle not inheritable
                    NULL,           // Thread handle not inheritable
                    FALSE,          // Set handle inheritance to FALSE
                    0,              // No creation flags
                    NULL,           // Use parent's environment block
                    NULL,           // Use parent's starting directory
                    &si,            // Pointer to STARTUPINFO structure
                    &pi)           // Pointer to PROCESS_INFORMATION structure
     ) {
    fprintf(stderr, "CreateProcess failed (%lu).\n", GetLastError());
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
