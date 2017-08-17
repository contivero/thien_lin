#include "util.h"

void
die(const char *errstr, ...){
    va_list ap;

    va_start(ap, errstr);
    vfprintf(stderr, errstr, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

FILE *
xfopen (const char *filename, const char *mode){
    FILE *fp = fopen(filename, mode);

    if(!fp)
        die("fopen: couldn't open %s\n", filename);

    return fp;
}

void
xfclose (FILE *fp){
    if(fclose(fp) == EOF)
        die("fclose: error\n");
}

void
xfread(void *ptr, size_t size, size_t nmemb, FILE *stream){
    if(fread(ptr, size, nmemb, stream) < 1)
        die("fread: error\n");
}

void
xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
    if(fwrite(ptr, size, nmemb, stream) != nmemb)
        die("fwrite: error in writing or end of file.\n");
}

void
xfseek(FILE *fp,  long offset,  int whence) {
    if(fseek(fp, offset, whence))
        die("fseek: error\n");
}

DIR *
xopendir(const char *name){
    DIR *dp = opendir(name);

    if(!dp)
        die("xopendir: error opening %s\n", name);

    return dp;
}

void
xclosedir(DIR *dirp){
    if(!dirp)
        die("xclosedir: null pointer\n");
    if(closedir(dirp))
        die("xclosedir: error\n");
}

void *
xmalloc(size_t size){
    void *p = malloc(size);

    if(!p)
        die("xmalloc: couldn't allocate %d bytes\n", size);

    return p;
}

int
xsnprintf(char *str, size_t size, const char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    int len = vsnprintf(str, size, fmt, ap);
    va_end(ap);
    if (len < 0)
        die("xsnprintf: error\n");
    if (len >= size)
        die("xsnprintf: snprintf buffer too small\n");

    return len;
}

bool
isbigendian(void){
    int value = 1;

    return *(char *)&value != 1;
}

/* Used to handle cases such as:
 * -1 % 10 == -1
 * when it should be:
 * -1 % 10 == 9
 */
int
mod(int a, int b){
    int m = a % b;

    return m < 0 ? m + b : m;
}

inline void
uint16swap(uint16_t *x){
    *x = *x >> 8 | *x << 8;
}

inline void
uint32swap(uint32_t *x){
    *x = ((*x & (uint32_t) 0x000000FFU) << 24) |
        ((*x & (uint32_t) 0x0000FF00U) <<  8) |
        ((*x & (uint32_t) 0x00FF0000U) >>  8) |
        ((*x & (uint32_t) 0xFF000000U) >> 24);
}

inline void
int32swap(int32_t *x){
    *x = ((*x << 8) & 0xFF00FF00) | ((*x >> 8) & 0xFF00FF);
    *x = (*x << 16) | ((*x >> 16) & 0xFFFF);
}
