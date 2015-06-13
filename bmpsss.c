#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BMP_HEADER_SIZE        14
#define DIB_HEADER_SIZE        40
#define PALETTE_SIZE           1024
#define PIXEL_ARRAY_OFFSET     BMP_HEADER_SIZE + DIB_HEADER_SIZE + PALETTE_SIZE
#define WIDTH_OFFSET           18
#define HEIGHT_OFFSET          22
#define PRIME                  251
#define RIGHTMOST_BIT_ON(x)    (x) |= 0x01
#define RIGHTMOST_BIT_OFF(x)   (x) &= 0xFE

typedef struct {
	uint8_t id[2];     /* magic number to identify the BMP format */
	uint32_t size;     /* size of the BMP file in bytes */
	uint16_t unused1;  /* key */
	uint16_t unused2;  /* shadow number */
	uint32_t offset;   /* starting address of the pixel array (bitmap data) */
} BMPheader;

/* 40 bytes BITMAPINFOHEADER */
typedef struct {
	uint32_t size;            /* the size of this header (40 bytes) */
	uint32_t width;           /* the bitmap width in pixels */
	uint32_t height;          /* the bitmap height in pixels */
	uint16_t nplanes;         /* number of color planes used; Must set to 1 */
	uint16_t depth;           /* bpp number. Usually: 1, 4, 8, 16, 24 or 32 */
	uint32_t compression;     /* compression method used */
	uint32_t pixelarraysize;  /* size of the raw bitmap (pixel) data */
	uint32_t hres;            /* horizontal resolution (pixel per meter) */
	uint32_t vres;            /* vertical resolution (pixel per meter) */
	uint32_t ncolors;         /* colors in the palette. 0 means 2^n */
	uint32_t nimpcolors;      /* important colors used, usually ignored */
} DIBheader;

typedef struct {
	BMPheader bmpheader;   /* 14 bytes bmp starting header */
	DIBheader dibheader;   /* 40 bytes dib header */
	uint8_t palette[1024]; /* color palette; mandatory for depth <= 8  */
	uint8_t *imgpixels;    /* array of bytes representing each pixel */
} Bitmap;

/* prototypes */ 
static void     die(const char *errstr, ...);
static void     *xmalloc(size_t size);
static FILE     *xfopen(const char *filename, const char *mode);
static void     xfclose(FILE *fp);
static void     xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
static void     xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
static void     usage(void);
static void     swap(uint8_t *s, uint8_t *t);
static int      mod(int dividend, int divisor);
static double   randnormalize(void);
static long     randint(long int max);
static uint32_t get32bitsfromheader(FILE *fp, int offset);
static uint32_t bmpfilewidth(FILE *fp);
static uint32_t bmpfileheight(FILE *fp);
static void     initpalette(uint8_t palette[]);
static Bitmap   *newbitmap(uint16_t width, uint16_t height);
static void     freebitmap(Bitmap *bp);
static void     readbmpheader(Bitmap *bp, FILE *fp);
static void     writebmpheader(Bitmap *bp, FILE *fp);
static void     readdibheader(Bitmap *bp, FILE *fp);
static void     writedibheader(Bitmap *bp, FILE *fp);
static Bitmap   *bmpfromfile(FILE *fp);
static void     validate_r(FILE *fp, int r);
static Bitmap   *loadbmp(const char *filename, int r);
static int      bmpimagesize(Bitmap *bp);
static void     bmptofile(Bitmap *bp, const char *filename);
static void     truncategrayscale(Bitmap *bp);
static void     permutepixels(Bitmap *bp, unsigned int seed);
static void     unpermutepixels(Bitmap *bp, unsigned int seed);
static uint8_t  generatepixel(const uint8_t *coeff, int degree, int value);
static void     findclosestpair(int value, uint16_t *v1, uint16_t *v2);
static Bitmap   *newshadow(uint16_t width, uint16_t height, unsigned int seed, uint16_t shadownumber);
static Bitmap   **formshadows(Bitmap *bp, unsigned int seed, int r, int n);
static void     findcoefficients(int **mat, int r);
static void     revealsecret(Bitmap **shadows, int r, int width, int height);

/* globals */
static char *argv0;                /* program name for usage() */
static const int modinv[PRIME] = { /* modular multiplicative inverse */
	0, 1, 126, 84, 63, 201, 42, 36, 157, 28, 226, 137, 21, 58, 18, 67, 204,
	192, 14, 185, 113, 12, 194, 131, 136, 241, 29, 93, 9, 26, 159, 81, 102,
	213, 96, 208, 7, 95, 218, 103, 182, 49, 6, 216, 97, 106, 191, 235, 68, 41,
	246, 64, 140, 90, 172, 178, 130, 229, 13, 234, 205, 107, 166, 4, 51, 112,
	232, 15, 48, 211, 104, 99, 129, 196, 173, 164, 109, 163, 177, 197, 91, 31,
	150, 124, 3, 189, 108, 176, 174, 110, 53, 80, 221, 27, 243, 37, 34, 44,
	146, 71, 123, 169, 32, 39, 70, 153, 45, 61, 86, 76, 89, 199, 65, 20, 240,
	227, 132, 118, 117, 135, 228, 195, 179, 100, 83, 249, 2, 168, 151, 72, 56,
	23, 116, 134, 133, 119, 24, 11, 231, 186, 52, 162, 175, 165, 190, 206, 98,
	181, 212, 219, 82, 128, 180, 105, 207, 217, 214, 8, 224, 30, 171, 198, 141,
	77, 75, 143, 62, 248, 127, 101, 220, 160, 54, 74, 88, 142, 87, 78, 55, 122,
	152, 147, 40, 203, 236, 19, 139, 200, 247, 85, 144, 46, 17, 238, 22, 121,
	73, 79, 161, 111, 187, 5, 210, 183, 16, 60, 145, 154, 35, 245, 202, 69,
	148, 33, 156, 244, 43, 155, 38, 149, 170, 92, 225, 242, 158, 222, 10, 115,
	120, 57, 239, 138, 66, 237, 59, 47, 184, 233, 193, 230, 114, 25, 223, 94,
	215, 209, 50, 188, 167, 125
};

void
die(const char *errstr, ...){
	va_list ap;

	va_start(ap, errstr);
	vfprintf(stderr, errstr, ap);
	va_end(ap);
	exit(EXIT_FAILURE);
}

void *
xmalloc(size_t size){
	void *p = malloc(size);

	if(!p)
		die("Out of memory: couldn't malloc %d bytes\n", size);

	return p;
}

FILE *
xfopen (const char *filename, const char *mode){
	FILE *fp = fopen(filename, mode);

	if(!fp) 
		die("couldn't open %s\n", filename);

	return fp;
}

void
xfclose (FILE *fp){
	if(fclose(fp) == EOF)
		die("couldn't close file\n");
}

void
xfread(void *ptr, size_t size, size_t nmemb, FILE *stream){
	if (fread(ptr, size, nmemb, stream) < 1)
		die("fread: error\n"); /* TODO print errno string! */
}

void 
xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
	if (fwrite(ptr, size, nmemb, stream) != nmemb)
		die("fwrite: error in writing or end of file.\n");
}

void
usage(void){
	die("usage: %s -(d|r) -secret image -k number [-n number|-dir directory]\n"
		"2 <= k <= n; 2 <= n\n", argv0);
}

void
swap(uint8_t *s, uint8_t *t){
	uint8_t temp;

	temp = *s;
	*s = *t;
	*t = temp;
}

int
mod(int dividend, int divisor){
	int modulus = dividend % divisor;

	return modulus > 0 ? modulus : modulus + divisor;
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
get32bitsfromheader(FILE *fp, int offset){
	uint32_t value;
	uint32_t pos = ftell(fp);

	fseek(fp, offset, SEEK_SET);
	xfread(&value, 4, 1, fp);
	fseek(fp, pos, SEEK_SET);

	return value;
}

uint32_t
bmpfilewidth(FILE *fp){
	return get32bitsfromheader(fp, WIDTH_OFFSET);
}

uint32_t
bmpfileheight(FILE *fp){
	return get32bitsfromheader(fp, HEIGHT_OFFSET);
}

/* initialize palette with default 8-bit greyscale values */
void
initpalette(uint8_t palette[]){
	int i, j;

	for(i = 0; i < 256; i++){
		j = i * 4; 
		palette[j++] = i;
		palette[j++] = i;
		palette[j++] = i;
		palette[j] = 0;
	}
}

Bitmap *
newbitmap(uint16_t width, uint16_t height){
	int totalpixels = width * height;
	Bitmap *bmp = xmalloc(sizeof(*bmp));

	bmp->bmpheader.id[0]   = 'B';
	bmp->bmpheader.id[1]   = 'M';
	bmp->bmpheader.size    = PIXEL_ARRAY_OFFSET + totalpixels;
	bmp->bmpheader.unused1 = 0;
	bmp->bmpheader.unused2 = 0;
	bmp->bmpheader.offset  = PIXEL_ARRAY_OFFSET;

	bmp->dibheader.size           = DIB_HEADER_SIZE;
	bmp->dibheader.width          = width; 
	bmp->dibheader.height         = height;
	bmp->dibheader.nplanes        = 1;  
	bmp->dibheader.depth          = 8;     
	bmp->dibheader.compression    = 0;
	bmp->dibheader.pixelarraysize = totalpixels;
	bmp->dibheader.hres           = 0;
	bmp->dibheader.vres           = 0;
	bmp->dibheader.ncolors        = 0;
	bmp->dibheader.nimpcolors     = 0;

	initpalette(bmp->palette);
	bmp->imgpixels = xmalloc(totalpixels);

	return bmp;
}

void
freebitmap(Bitmap *bp){
	free(bp->imgpixels);
	free(bp);
}

void
bmpheaderdebug(Bitmap *bp){
	printf("ID: %c%-15c size: %-16d r1: %-16d r2: %-16d offset: %-16d\n",
			bp->bmpheader.id[0], bp->bmpheader.id[1], bp->bmpheader.size, 
			bp->bmpheader.unused1, bp->bmpheader.unused2, bp->bmpheader.offset);
}

void
dibheaderdebug(Bitmap *bp){
	printf("dibsize: %-16d width: %-16d height: %-16d\n" 
			"nplanes: %-16d depth: %-16d compression:%-16d\n"
			"pixelarraysize: %-16d hres: %-16d vres:%-16d\n"
			"ncolors: %-16d nimpcolors: %-16d\n", bp->dibheader.size,
			bp->dibheader.width, bp->dibheader.height, bp->dibheader.nplanes,
			bp->dibheader.depth, bp->dibheader.compression,
			bp->dibheader.pixelarraysize, bp->dibheader.hres, bp->dibheader.vres,
			bp->dibheader.ncolors, bp->dibheader.nimpcolors);
}

void
readbmpheader(Bitmap *bp, FILE *fp){
	xfread(&bp->bmpheader.id, 1, sizeof(bp->bmpheader.id), fp);
	xfread(&bp->bmpheader.size, 1, sizeof(bp->bmpheader.size), fp);
	xfread(&bp->bmpheader.unused1, 1, sizeof(bp->bmpheader.unused1), fp);
	xfread(&bp->bmpheader.unused2, 1, sizeof(bp->bmpheader.unused2), fp);
	xfread(&bp->bmpheader.offset, 1, sizeof(bp->bmpheader.offset), fp);
}

void
writebmpheader(Bitmap *bp, FILE *fp){
	xfwrite(&bp->bmpheader.id, 1, sizeof(bp->bmpheader.id), fp);
	xfwrite(&bp->bmpheader.size, 1, sizeof(bp->bmpheader.size), fp);
	xfwrite(&bp->bmpheader.unused1, 1, sizeof(bp->bmpheader.unused1), fp);
	xfwrite(&bp->bmpheader.unused2, 1, sizeof(bp->bmpheader.unused2), fp);
	xfwrite(&bp->bmpheader.offset, 1, sizeof(bp->bmpheader.offset), fp);
}

void
readdibheader(Bitmap *bp, FILE *fp){
	xfread(&bp->dibheader.size, 1, sizeof(bp->dibheader.size), fp);
	xfread(&bp->dibheader.width, 1, sizeof(bp->dibheader.width), fp);
	xfread(&bp->dibheader.height, 1, sizeof(bp->dibheader.height), fp);
	xfread(&bp->dibheader.nplanes, 1, sizeof(bp->dibheader.nplanes), fp);
	xfread(&bp->dibheader.depth, 1, sizeof(bp->dibheader.depth), fp);
	xfread(&bp->dibheader.compression, 1, sizeof(bp->dibheader.compression), fp);
	xfread(&bp->dibheader.pixelarraysize, 1, sizeof(bp->dibheader.pixelarraysize), fp);
	xfread(&bp->dibheader.hres, 1, sizeof(bp->dibheader.hres), fp);
	xfread(&bp->dibheader.vres, 1, sizeof(bp->dibheader.vres), fp);
	xfread(&bp->dibheader.ncolors, 1, sizeof(bp->dibheader.ncolors), fp);
	xfread(&bp->dibheader.nimpcolors, 1, sizeof(bp->dibheader.nimpcolors), fp);
}

void
writedibheader(Bitmap *bp, FILE *fp){
	xfwrite(&bp->dibheader.size, 1, sizeof(bp->dibheader.size), fp);
	xfwrite(&bp->dibheader.width, 1, sizeof(bp->dibheader.width), fp);
	xfwrite(&bp->dibheader.height, 1, sizeof(bp->dibheader.height), fp);
	xfwrite(&bp->dibheader.nplanes, 1, sizeof(bp->dibheader.nplanes), fp);
	xfwrite(&bp->dibheader.depth, 1, sizeof(bp->dibheader.depth), fp);
	xfwrite(&bp->dibheader.compression, 1, sizeof(bp->dibheader.compression), fp);
	xfwrite(&bp->dibheader.pixelarraysize, 1, sizeof(bp->dibheader.pixelarraysize), fp);
	xfwrite(&bp->dibheader.hres, 1, sizeof(bp->dibheader.hres), fp);
	xfwrite(&bp->dibheader.vres, 1, sizeof(bp->dibheader.vres), fp);
	xfwrite(&bp->dibheader.ncolors, 1, sizeof(bp->dibheader.ncolors), fp);
	xfwrite(&bp->dibheader.nimpcolors, 1, sizeof(bp->dibheader.nimpcolors), fp);
}

Bitmap *
bmpfromfile(FILE *fp){
	Bitmap *bp = xmalloc(sizeof(*bp));

	readbmpheader(bp, fp);
	readdibheader(bp, fp);

	/* TODO: debug info, delete later! */
	bmpheaderdebug(bp);
	dibheaderdebug(bp);

	/* read color palette */
	xfread(bp->palette, 1, PALETTE_SIZE, fp);

	/* read pixel data */
	int imagesize = bmpimagesize(bp);
	bp->imgpixels = xmalloc(imagesize);
	xfread(bp->imgpixels, 1, imagesize, fp);

	return bp;
}

void
validate_r(FILE *fp, int r){
	int width  = bmpfilewidth(fp);
	int height = bmpfileheight(fp);
	int pixels = width * height;
	int aux    = pixels / r;

	if(pixels != aux * r)
		die("Can't divide %dx%d image by %d. Choose another r\n", 
				width, height, r);
}

Bitmap *
loadbmp(const char *filename, int r){
	FILE *fp = xfopen(filename, "r");

	validate_r(fp, r);
	Bitmap *bmp = bmpfromfile(fp);
	xfclose(fp);

	return bmp;
}

int
bmpimagesize(Bitmap *bp){
	return bp->bmpheader.size - bp->bmpheader.offset;
}

void
bmptofile(Bitmap *bp, const char *filename){
	FILE *fp = xfopen(filename, "w");

	writebmpheader(bp, fp);
	writedibheader(bp, fp);
	xfwrite(bp->palette, PALETTE_SIZE, 1, fp);
	xfwrite(bp->imgpixels, bmpimagesize(bp), 1, fp);
	xfclose(fp);
}

void
truncategrayscale(Bitmap *bp){
	for(int i = 0; i < PALETTE_SIZE; i++)
		if(bp->palette[i] > 250)
			bp->palette[i] = 250;
}

void
permutepixels(Bitmap *bp, unsigned int seed){
	int i, j;
	uint8_t *p = bp->imgpixels;
	int imgsize = bmpimagesize(bp);

	for(i = imgsize - 1; i > 1; i--){
		/* j = permutationseq[i]; */
		j = randint(i);
		swap(&p[j], &p[i]);
	}
}

void
unpermutepixels(Bitmap *bp, unsigned int seed){
	int i, j;
	uint8_t *p = bp->imgpixels;
	int imgsize = bmpimagesize(bp);
	int *permseq = xmalloc(sizeof(*permseq) * imgsize);

	srand(seed);

	for(i = imgsize - 1; i > 0; i--)
		permseq[i] = randint(i);

	for(i = 1 ; i < imgsize - 1; i++){
		j = permseq[i];
		swap(&p[j], &p[i]);
	}
}

/* uses coeff[0] to coeff[degree] to evaluate the corresponding
 * section polynomial and generate a pixel for a shadow image */
uint8_t
generatepixel(const uint8_t *coeff, int degree, int value){
	long ret = 0;

	for(int i = 0; i <= degree; i++)
		ret += coeff[i] * powl(value,i);

	return ret % PRIME;
}

void
findclosestpair(int x, uint16_t *width, uint16_t *height){
	int n = floor(sqrt(x));

	for(; n > 2; n--){
		if(x % n == 0){
			*width  = n;
			*height = x / n;
			break;
		}
	}
}

Bitmap *
newshadow(uint16_t width, uint16_t height, unsigned int seed, uint16_t shadownumber){
	Bitmap *bmp = newbitmap(width, height);
	bmp->bmpheader.unused2 = shadownumber;

	return bmp;
}

Bitmap **
formshadows(Bitmap *bp, unsigned int seed, int r, int n){
	uint16_t width, height;
	uint8_t *coeff;
	int i, j;
	int totalpixels = bmpimagesize(bp);
	Bitmap **shadows = xmalloc(sizeof(*shadows) * n);

	findclosestpair(totalpixels/r, &width, &height);

	for(i = 0; i < n; i++)
		shadows[i] = newshadow(width, height, seed, i+1);

	/* generate shadow image pixels */
	for(j = 0; j*r < totalpixels; j++){
		coeff = &bp->imgpixels[j*r]; 
		for(i = 0; i < n; i++){
			shadows[i]->imgpixels[j] = generatepixel(coeff, r-1, i+1);
		}
	}

	return shadows;
}

/* debugging function: TODO delete later! */
void
printmat(int **mat, int rows, int cols){
	int i, j;

	for(i = 0; i < rows; i++){
		printf("|");
		for(j = 0; j < cols; j++){
			printf("%d ", mat[i][j]);
		}
		printf("|\n");
	}
}

void
findcoefficients(int **mat, int r){
	int i, j, k, a, temp;

	/* take matrix to echelon form */
	for(j = 0; j < r-1; j++){
		for(i = r-1; i > j; i--){
			a = mat[i][j] * modinv[mat[i-1][j]];
			for(k = j; k < r+1; k++){
				temp = mat[i][k] - ((mat[i-1][k] * a) % PRIME);
				mat[i][k] = mod(temp, PRIME);
			}
		}
	}

	/* take matrix to reduced row echelon form */
	for(i = r-1; i > 0; i--){
		mat[i][r] = (mat[i][r] * modinv[mat[i][i]]) % PRIME;
		mat[i][i] = 1;
		for(k = i-1; k >= 0; k--){
			temp = mat[k][r] - ((mat[i][r] * mat[k][i]) % PRIME);
			mat[k][r] = mod(temp, PRIME);
			mat[k][i] = 0;
		}
	}
}

void
revealsecret(Bitmap **shadows, int r, int width, int height){
	int i, j, k, value;
	int pixels = (*shadows)->dibheader.pixelarraysize;
	Bitmap *bmp = newbitmap(width, height);
	Bitmap *sp;

	int **mat = xmalloc(sizeof(*mat) * r);
	for(i = 0; i < r; i++)
		mat[i] = xmalloc(sizeof(**mat) * (r+1));

	for(i = 0; i < pixels; i++){
		for(j = 0; j < r; j++){
			sp = shadows[j];
			value = sp->bmpheader.unused2;
			mat[j][0] = 1;
			for(k = 1; k < r; k++){
				mat[j][k] = value;
				value *= sp->bmpheader.unused2;
			}
			mat[j][r] = sp->imgpixels[i];
		}
		findcoefficients(mat, r);
		for(j = i * r; j < (i+1) * r; j++){
			bmp->imgpixels[j] = mat[j % r][r];
		}
	} 
	/* unpermutepixels(bmp, sp->bmpheader.unused1); */
	bmptofile(bmp, "revealed.bmp");

	for(i = 0; i < r; i++)
		free(mat[i]);
	free(mat);
	freebitmap(bmp);
}

void
hideshadow(Bitmap *bp, Bitmap *shadow){
	int pixels = bmpimagesize(shadow);
	int i, j;
	uint8_t byte;
	char shadowfilename[20] = {0};
	
	bp->bmpheader.unused1 = shadow->bmpheader.unused1;
	bp->bmpheader.unused2 = shadow->bmpheader.unused2;
	snprintf(shadowfilename, 20, "shadow%d.bmp", shadow->bmpheader.unused2);
	printf("%s\n", shadowfilename);

	for(i = 0; i < pixels; i++){
		byte = shadow->imgpixels[i];
		for(j = i*8; j < 8*(i+1); j++){
			if(byte & 0x80) /* 1000 0000 */
				RIGHTMOST_BIT_ON(bp->imgpixels[j]);
			else
				RIGHTMOST_BIT_OFF(bp->imgpixels[j]);
			byte <<= 1;
		}
	}
	bmptofile(bp, shadowfilename);
}

/* width and height parameters needed because the image hiding the shadow could
 * be bigger than necessary */
Bitmap *
retrieveshadow(Bitmap *bp, uint16_t width, uint16_t height, int r){
	int i, j;
	uint8_t byte, mask;

	if(bp->dibheader.width * bp->dibheader.height < ((width * height)*8)/r)
		die("image of %d pixels can't contain shadow of %d pixels\n",
				bp->dibheader.width * bp->dibheader.height, ((width * height)*8)/r);

	findclosestpair((width * height)/r, &width, &height);
	Bitmap *shadow = newshadow(width, height, bp->bmpheader.unused1, bp->bmpheader.unused2);
	int shadowpixels = shadow->dibheader.pixelarraysize;

	for(i = 0; i < shadowpixels; i++){
		byte = 0;
		mask = 0x80; /* 1000 0000 */
		for(j = i*8; j < 8*(i+1); j++){
			if(bp->imgpixels[j] & 0x01)
				byte |= mask;
			mask >>= 1;
		}
		shadow->imgpixels[i] = byte;
	}

	return shadow;
}

int
main(int argc, char *argv[]){
	char *filename;
	int i;

	/* keep program name for usage() */
	argv0 = argv[0]; 

	/* TODO receive as parameter and parse */
	int r = 8, n = 8, width = 300, height = 300, seed = 7;

	filename = argv[1];
	Bitmap **bmps = xmalloc(sizeof(*bmps) * r);

	for(i = 0; i < r; i++){
		bmps[i] = loadbmp(argv[i+1], r);
		bmps[i] = retrieveshadow(bmps[i], width, height, r);
	}

	/* truncategrayscale(bp); */
	/* Bitmap **shadows = formshadows(bp, r, n); */

	revealsecret(bmps, r, width, height);

	/* for(n -= 1; n >= 0; n--) */
	/* 	freebitmap(shadows[n]); */ 
	/* free(shadows); */
	/* freebitmap(bp); */
}
