#define _CRT_SECURE_NO_WARNINGS 1

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

int vasprintf(char **strp, const char *fmt, va_list ap) {
    // _vscprintf tells you how big the buffer needs to be
    int len = _vscprintf(fmt, ap);
    if (len == -1) {
        return -1;
    }
    size_t size = (size_t)len + 1;
    char *str = malloc(size);
    if (!str) {
        return -1;
    }
    // _vsprintf_s is the "secure" version of vsprintf
    int r = vsprintf_s(str, len + 1, fmt, ap);
    if (r == -1) {
        free(str);
        return -1;
    }
    *strp = str;
    return r;
}

int asprintf(char **strp, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int r = vasprintf(strp, fmt, ap);
    va_end(ap);
    return r;
}

/* Return a newly allocated copy of STRING.  */
char * xstrdup(const char *string)
{
    return strcpy(malloc(strlen(string) + 1), string);
}

//void* xrealloc(void*  _Block, size_t _Size)
//{
//    return realloc(_Block, _Size);
//}
//
//void *xcalloc(size_t nmemb, size_t size)
//{
//    return calloc(nmemb, size);
//}
//
//void *xmalloc(size_t size)
//{
//    return malloc(size);
//}

size_t Fread(void *buffer, size_t size, size_t nmemb, FILE *f)
{
#ifdef _DEBUG
	//long int cur_pos = ftell(f);
	//if (cur_pos >= 0)
	//{
	//	fseek(f, 0, SEEK_END);
	//	long int len = ftell(f);
	//	fseek(f, cur_pos, SEEK_SET);
	//	long int new_pos = ftell(f);
	//	if (new_pos != cur_pos)
	//		new_pos = new_pos;
	//}
#endif
	size_t bytesRead = 0;
	size_t bytesTotal = size*nmemb;
	size_t bytesRemained = bytesTotal;
	while (bytesRead < bytesTotal)
	{
		if (feof(f))
			break;
		if (ferror(f))
			break;	
		size_t result = fread(buffer, 1, bytesRemained, f);
		if (!result)
			break;
		bytesRead += result;
		bytesRemained -= result;
	}
	return bytesRead;
}