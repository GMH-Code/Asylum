#if defined(__HAIKU__) || defined(_WIN32)
#define HAVE_GET_EXE_PATH
char *get_exe_path(char *buf, int bufsize);
#endif
