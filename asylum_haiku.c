#include <string.h>
#include <os/kernel/image.h>

char *get_exe_path(char *buf, int bufsize)
{
	int r;
	int32 cookie;
	image_info info;
	char *p;

	cookie = 0;
	r = get_next_image_info(0, &cookie, &info);
	if( r == B_OK ) {
		strncpy(buf, info.name, bufsize);
		buf[bufsize-1] = '\0';
		p = strrchr(buf, '/');
		if (p)
			*p = '\0';
	} else {
		return NULL;
	}

	return buf;
}

