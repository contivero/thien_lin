#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>

void     die(const char *errstr, ...);
void     xfclose(FILE *fp);
FILE     *xfopen(const char *filename, const char *mode);
void     xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
void     xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
DIR      *xopendir(const char *name);
void     *xmalloc(size_t size);

int      mod(int a, int b);
int      isbigendian(void);
void     uint16swap(uint16_t *x);
void     uint32swap(uint32_t *x);
int32_t  int32swap(int32_t *x);
