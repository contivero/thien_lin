#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <dirent.h>

void die(const char *errstr, ...);

void xfclose(FILE *fp);
FILE *xfopen(const char *filename, const char *mode);
void xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
void xfseek(FILE *fp, long offset, int whence);
DIR  *xopendir(const char *name);
void xclosedir(DIR *dirp);
void *xmalloc(size_t size);
int  xsnprintf(char *str, size_t size, const char *fmt, ...);

int  mod(int a, int b);

bool isbigendian(void);
void uint16swap(uint16_t *x);
void uint32swap(uint32_t *x);
void int32swap(int32_t *x);
