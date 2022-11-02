#include <windows.h>

char *get_exe_path(char *buf, int bufsize)
{
	char *p;
	GetModuleFileName(NULL, buf, bufsize);
	while (p = strchr(buf, '\\'))
		*p = '/';
	p = strrchr(buf, '/');
	if (p)
		*p = '\0';
	buf[bufsize-1] = '\0';

	return buf;
}

