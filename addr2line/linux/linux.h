#ifndef _LINUX_PORT__H
#define _LINUX_PORT__H

int asprintf(char **strp, const char *fmt, ...);
char *xstrdup(const char *string);
void *xrealloc(void*  _Block, size_t _Size);
void *xcalloc(size_t nmemb, size_t size);
void *xmalloc(size_t size);
size_t Fread(void *ptr, size_t size, size_t nmemb, FILE *stream);

//#define dlopen LoadLibraryA
//#define dlsym GetProcAddressA

#endif /* _LINUX_PORT__H */
