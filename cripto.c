#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#define WIDTH_OFFSET      18
#define WIDTH_FIELD_SIZE  4
#define HEIGHT_OFFSET     22
#define HEIGHT_FIELD_SIZE 4
#define HEADER_SIZE       54
#define BMP_HEADER_SIZE   14

typedef enum {
	BITMAPCOREHEADER   = 12,
	OS21XBITMAPHEADER  = 12,
	OS22XBITMAPHEADER  = 64,
	BITMAPINFOHEADER   = 40,
	BITMAPV2INFOHEADER = 52,
	BITMAPV3INFOHEADER = 56,
	BITMAPV4HEADER     = 108,
	BITMAPV5HEADER     = 124,
} DIBheadersize;

typedef struct {
	uint8_t id[2];    /* magic number to identify the BMP format */
	uint32_t size;    /* size of the BMP file in bytes */
	uint16_t unused1; /* reserved */
	uint16_t unused2; /* reserved */
	uint32_t offset;  /* starting address of the pixel array (bitmap data) */
} BMPheader;

typedef struct {
	BMPheader bmpheader[BMP_HEADER_SIZE];
	char *dibheader;
	uint8_t *data;
} Bitmap;

/* prototypes */ 
void		randomize(int num);
double		randnormalize(void);
long int	randint(long int max);
uint32_t	bmpwidth(FILE *fp);
uint32_t	bmpheight(FILE *fp);
void		*newbitmap(uint32_t width, uint32_t height, DIBheadersize n);
void		*freebitmap(Bitmap *bp);
void		die(const char *errstr, ...);
void		*xmalloc(size_t size);
FILE		*xfopen(const char *filename, const char *mode);
void		xfclose(FILE *fp, const char *filename);

void
die(const char *errstr, ...) {
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void *
xmalloc(size_t size) {
	void *p = malloc(size);

	if(!p)
		die("Out of memory: couldn't malloc %d bytes\n", size);

	return p;
}

FILE *
xfopen (const char *filename, const char *mode){
	FILE *fp = fopen(filename, mode);

	if(!fp) 
		die("couldn't open %s", filename);

	return fp;
}

void
xfclose (FILE *fp, const char *filename){
	if(fclose(fp) == EOF)
		die("couldn't close %s", filename);
}

void *
newbitmap(uint32_t width, uint32_t height, DIBheadersize n){
	Bitmap *bp    = xmalloc(sizeof(*bp));
	bp->dibheader = xmalloc(n);
	bp->data      = xmalloc(width * height);
	return bp;
}

void *
freebitmap(Bitmap *bp){
	free(bp->dibheader);
	free(bp->data);
	free(bp);
}

/*
Bitmap *
bmpfromfile(char *filename){
	int i;
	FILE *fp = xfopen(filename, "r");
	int size = bmpfilewidth(fp)
		Bitmap *bp = newbitmap(bmpfilewidth(fp), bmpfileheight(fp));
	for(i = 0; i < size; i++){
		fread(bp->header, 1, ) 

	}
	bp->header
		xclose(fp);

	fread(&width, 1, WIDTH_FIELD_SIZE, fp);
}
*/

void 
randomize(int num){
	srand((int) num);
}

double 
randnormalize(void){
	return rand()/((double)RAND_MAX+1);
}

long
randint(long int max){
	return (long) (randnormalize()*(max+1)); /*returns num in [0,max]*/
} 

uint32_t
bmpwidth(FILE *fp){
	uint32_t width;
	uint32_t pos = ftell(fp);

	fseek(fp, WIDTH_OFFSET, SEEK_SET);
	fread(&width, 1, WIDTH_FIELD_SIZE, fp);
	fseek(fp, pos, SEEK_SET);

	return width;
}

uint32_t
bmpheight(FILE *fp){
	uint32_t height;
	uint32_t pos = ftell(fp);

	fseek(fp, HEIGHT_OFFSET, SEEK_SET);
	fread(&height, 1, HEIGHT_FIELD_SIZE, fp);
	fseek(fp, pos, SEEK_SET);

	return height;
}

void
changebmppixel(FILE *fp, int pixel, char bit){
	uint8_t byte;

	//fseek(fp, HEIGHT_OFFSET, SEEK_SET);
	fread(&byte, 1, 1, fp);
	byte &= bit;
	fseek(fp, -1, SEEK_CUR);
	fwrite(&byte, 1, 1, fp);

}

int
main(int argc, char *argv[]){
	char *filename;
	char *arg;
	FILE *fp;

	/*
	   if(strcmp("-secret", arg) == 0){

	   }
	   */

	filename = argv[1];
	if(!(fp = fopen(filename, "r")))
		return EXIT_FAILURE;
	printf("width: %d, height: %d\n", bmpwidth(fp), bmpheight(fp));
}
