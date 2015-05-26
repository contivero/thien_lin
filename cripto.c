#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BMP_HEADER_SIZE        14
#define DIB_HEADER_SIZE_OFFSET 14
#define DIB_HEADER_SIZE        40
#define FILESIZE_OFFSET        2	
#define WIDTH_OFFSET           18
#define HEIGHT_OFFSET          22
#define PRIME_MOD              251

#pragma pack(1)
typedef struct {
	uint8_t id[2];    /* magic number to identify the BMP format */
	uint32_t size;    /* size of the BMP file in bytes */
	uint16_t unused1; /* reserved */
	uint16_t unused2; /* reserved */
	uint32_t offset;  /* starting address of the pixel array (bitmap data) */
} BMPheader;
#pragma pack()

/* 40 bytes BITMAPINFOHEADER */
typedef struct {
	uint32_t size;        /* the size of this header (40 bytes) */
	uint32_t width;       /* the bitmap width in pixels */
	uint32_t height;      /* the bitmap height in pixels */
	uint16_t nplanes;     /* number of color planes used; Must set to 1 */
	uint16_t depth;       /* bpp number. Usually: 1, 4, 8, 16, 24 or 32 */
	uint32_t compression; /* compression method used */
	uint32_t rawbmpsize;  /* size of the raw bitmap (pixel) data */
	uint32_t hres;        /* horizontal resolution (pixel per meter) */
	uint32_t vres;        /* vertical resolution (pixel per meter) */
	uint32_t ncolors;     /* number of colors in the palette. 0 means 2^n */
	uint32_t nimpcolors;  /* number of important colors used, usually ignored */
} DIBheader;

typedef struct {
	BMPheader bmpheader;  /* 14 bytes bmp starting header */
	DIBheader dibheader;  /* 40 bytes dib header */
	uint8_t *palette;     /* color palette */
	uint8_t *imgpixels;        /* image pixels */
} Bitmap;

/* prototypes */ 
static void       die(const char *errstr, ...);
static void       *xmalloc(size_t size);
static FILE       *xfopen(const char *filename, const char *mode);
static void       xfclose(FILE *fp);
static void       usage(void);
static long       randint(long int max);
static double     randnormalize(void);
static int        generatepixel(uint8_t *coeff, int degree, int value);
static uint32_t   get32bitsfromheader(FILE *fp, char offset);
static uint32_t   bmpfilesize(FILE *fp);
static uint32_t   bmpfilewidth(FILE *fp);
static uint32_t   bmpfileheight(FILE *fp);
static uint32_t   bmpfiledibheadersize(FILE *fp);
static void       freebitmap(Bitmap *bp);
static Bitmap     *bmpfromfile(char *filename);
static void       bmptofile(Bitmap *bp, const char *filename);
static int        bmpimagesize(Bitmap *bp);
static int        bmppalettesize(Bitmap *bp);
static Bitmap     **formshadows(Bitmap *bp, int r, int n);

/* globals */
static char *argv0; /* program name for usage() */

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
xfclose (FILE *fp){
	if(fclose(fp) == EOF)
		die("couldn't close file\n");
}

void
usage(void){
	die("usage: %s -(d|r) -secret image -k number [-n number|-dir directory]\n"
		"2 <= k <= n; 2 <= n\n", argv0);
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
get32bitsfromheader(FILE *fp, char offset){
	uint32_t value;
	uint32_t pos = ftell(fp);

	fseek(fp, offset, SEEK_SET);
	fread(&value, 4, 1, fp);
	fseek(fp, pos, SEEK_SET);

	return value;
}

uint32_t
bmpfilesize(FILE *fp){
	return get32bitsfromheader(fp, FILESIZE_OFFSET);
}

uint32_t
bmpfilewidth(FILE *fp){
	return get32bitsfromheader(fp, WIDTH_OFFSET);
}

uint32_t
bmpfileheight(FILE *fp){
	return get32bitsfromheader(fp, HEIGHT_OFFSET);
}

uint32_t
bmpfiledibheadersize(FILE *fp){
	uint32_t size = get32bitsfromheader(fp, DIB_HEADER_SIZE_OFFSET);
	if(size != DIB_HEADER_SIZE)
		die("unsupported dib header format\n");
	return size;
}

void
freebitmap(Bitmap *bp){
	free(bp->palette);
	free(bp->imgpixels);
	free(bp);
}

void
bmpheaderdebug(Bitmap *bp){
	printf("ID: %c%-15c size: %-16d offset: %-16d\n", bp->bmpheader.id[0],
	bp->bmpheader.id[1], bp->bmpheader.size, bp->bmpheader.offset);
}

void
dibheaderdebug(Bitmap *bp){
	printf("dibsize: %-16d width: %-16d height: %-16d\n" 
			"nplanes: %-16d depth: %-16d compression:%-16d\n"
			"rawbmpsize: %-16d hres: %-16d vres:%-16d\n"
			"ncolors: %-16d nimpcolors: %-16d\n", bp->dibheader.size,
			bp->dibheader.width, bp->dibheader.height, bp->dibheader.nplanes,
			bp->dibheader.depth, bp->dibheader.compression,
			bp->dibheader.rawbmpsize, bp->dibheader.hres, bp->dibheader.vres,
			bp->dibheader.ncolors, bp->dibheader.nimpcolors);
}

Bitmap *
bmpfromfile(char *filename){
	FILE *fp = xfopen(filename, "r");
	Bitmap *bp = xmalloc(sizeof(*bp));

	/* read BMP header */
	int i = 0;
	while(i < BMP_HEADER_SIZE)
		i += fread(&(bp->bmpheader) + i, 1, BMP_HEADER_SIZE, fp);

	/* read DIB header */
	int dibheadersize = bmpfiledibheadersize(fp);
	i = 0;
	while(i < dibheadersize)
		i += fread(&bp->dibheader + i, 1, dibheadersize, fp);

	bmpheaderdebug(bp);
	dibheaderdebug(bp);

	/* read color palette */
	int palettesize = bmppalettesize(bp);
	bp->palette = xmalloc(palettesize);
	i = 0;
	while(i < palettesize)
		i += fread(bp->palette + i, 1, palettesize, fp);

	/* read pixel data */
	int imagesize = bmpimagesize(bp);
	bp->imgpixels = xmalloc(imagesize);
	i = 0;
	while(i < imagesize)
		i += fread(bp->imgpixels + i, 1, imagesize, fp);

	xfclose(fp);
	return bp;
}

int
bmpimagesize(Bitmap *bp){
	return bp->bmpheader.size - bp->bmpheader.offset;
}

int
bmppalettesize(Bitmap *bp){
	return bp->bmpheader.offset - (BMP_HEADER_SIZE + bp->dibheader.size);
}

void
bmptofile(Bitmap *bp, const char *filename){
	FILE *fp = xfopen(filename, "w");

	fwrite(&bp->bmpheader, sizeof(bp->bmpheader), 1, fp);
	fwrite(&bp->dibheader, sizeof(bp->dibheader), 1, fp);
	fwrite(bp->palette, bmppalettesize(bp), 1, fp);
	fwrite(bp->imgpixels, bmpimagesize(bp), 1, fp);
	xfclose(fp);
}

void
truncategrayscale(Bitmap *bp){
	int i;
	for(i = 0; i < 1024; i++)
		if(bp->palette[i] > 250)
			bp->palette[i] = 250;
}

/* Durstenfeld algorithm */
void
permutepixels(Bitmap *bp){
	int i, j, temp;
	uint8_t *p = bp->imgpixels;
	srand(10); /* TODO preguntar sobre la "key" de permutación! */

	for(i = bmpimagesize(bp)-1; i > 1; i--){
		j = randint(i);
		temp = p[j];
		p[j] = p[i];
		p[i] = temp;
	}
}

/* uses coeff[0] to coeff[degree] to evaluate the corresponding section
 * polynomial and generate a pixel for a shadow image */
int
generatepixel(uint8_t *coeff, int degree, int value){
	int i; 
	long ret = 0;

	for(i = 0; i <= degree; i++)
		ret += coeff[i] * powl(value,i);

	return ret % PRIME_MOD;
}

Bitmap **
formshadows(Bitmap *bp, int r, int n){
	uint8_t *coeff;
	int i, j = 0;
	int processedpixels = 0;
	char filename[256];
	Bitmap **shadows = xmalloc(sizeof(*shadows) * n);

	/* allocate memory for shadows and copy necessary data */
	int totalpixels = bmpimagesize(bp);
	for(i = 0; i < n; i++){
		shadows[i] = xmalloc(sizeof(**shadows));
		shadows[i]->palette = xmalloc(bmppalettesize(bp));
		shadows[i]->imgpixels = xmalloc(totalpixels/r);
		memcpy(&shadows[i]->bmpheader, &bp->bmpheader, BMP_HEADER_SIZE);
		memcpy(&shadows[i]->dibheader, &bp->dibheader, DIB_HEADER_SIZE);
		shadows[i]->dibheader.height = bp->dibheader.height/r;
		memcpy(shadows[i]->palette, &bp->palette, bmppalettesize(bp));
	}

	/* generate shadow image pixels */
	while(processedpixels < totalpixels){
		for(i = 0; i < n; i++){
			coeff = &bp->imgpixels[processedpixels]; 
			shadows[i]->imgpixels[j] = generatepixel(coeff, r-1, i+1);
		}
		processedpixels += r;
		j++;
	}

	return shadows;
}

int
main(int argc, char *argv[]){
	char *filename;
	char *arg;
	FILE *fp;

	/* keep program name for usage() */
	argv0 = argv[0]; 

	filename = argv[1];
	Bitmap *bp = bmpfromfile(filename);
	truncategrayscale(bp);
	permutepixels(bp);
	int r = 2, n = 4; /* TODO receive as parameter and parse */
	Bitmap **shadows = formshadows(bp, r, n);

	/* write shadows to disk */
	for(n -= 1; n >= 0; n--){
		snprintf(filename, 256, "shadow%d.bmp", n);
		printf("%s\n", filename);
		bmptofile(shadows[n], filename);
		freebitmap(shadows[n]);
	}
	free(shadows);
}
