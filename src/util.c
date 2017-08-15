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
	if (fread(ptr, size, nmemb, stream) < 1)
		die("fread: error\n");
}

void
xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
	if (fwrite(ptr, size, nmemb, stream) != nmemb)
		die("fwrite: error in writing or end of file.\n");
}

DIR *
xopendir(const char *name){
	DIR *dp = opendir(name);

	if(!dp)
		die("opendir: error\n");

	return dp;
}

void *
xmalloc(size_t size){
	void *p = malloc(size);

	if(!p)
		die("Out of memory: couldn't malloc %d bytes\n", size);

	return p;
}

int
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

inline int32_t
int32swap(int32_t *x){
    *x = ((*x << 8) & 0xFF00FF00) | ((*x >> 8) & 0xFF00FF); 
    return (*x << 16) | ((*x >> 16) & 0xFFFF);
}
