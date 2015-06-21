#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#define BMP_HEADER_SIZE        14
#define DIB_HEADER_SIZE        40
#define PALETTE_SIZE           1024
#define PIXEL_ARRAY_OFFSET     BMP_HEADER_SIZE + DIB_HEADER_SIZE + PALETTE_SIZE
#define WIDTH_OFFSET           18
#define HEIGHT_OFFSET          22
#define PRIME                  251
#define DEFAULT_SEED           691
#define BMP_MAGIC_NUMBER       0x424D      
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
	BMPheader bmpheader;           /* 14 bytes bmp starting header */
	DIBheader dibheader;           /* 40 bytes dib header */
	uint8_t palette[PALETTE_SIZE]; /* color palette; mandatory for depth <= 8 */
	uint8_t *imgpixels;            /* array of bytes representing each pixel */
} Bitmap;

/* prototypes */ 
static int      isbigendian(void);
static void     uint16swap(uint16_t *x);
static void     uint32swap(uint32_t *x);
static void     die(const char *errstr, ...);
static void     *xmalloc(size_t size);
static FILE     *xfopen(const char *filename, const char *mode);
static void     xfclose(FILE *fp);
static void     xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
static void     xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
static DIR      *xopendir(const char *name);
static void     usage(void);
static void     swap(uint8_t *s, uint8_t *t);
static int      mod(int a, int b);
static double   randnormalize(void);
static long     randint(long max);
static uint32_t get32bitsfromheader(FILE *fp, int offset);
static uint32_t bmpfilewidth(FILE *fp);
static uint32_t bmpfileheight(FILE *fp);
static void     initpalette(uint8_t palette[]);
static Bitmap   *newbitmap(uint32_t width, uint32_t height);
static void     freebitmap(Bitmap *bp);
static void     changeheaderendianness(BMPheader *h);
static void     changedibendianness(DIBheader *h);
static void     readbmpheader(Bitmap *bp, FILE *fp);
static void     writebmpheader(Bitmap *bp, FILE *fp);
static void     readdibheader(Bitmap *bp, FILE *fp);
static void     writedibheader(Bitmap *bp, FILE *fp);
static Bitmap   *bmpfromfile(const char *filename);
static int      validate_k(FILE *fp, int r);
static int      bmpimagesize(Bitmap *bp);
static void     bmptofile(Bitmap *bp, const char *filename);
static void     truncategrayscale(Bitmap *bp);
static void     permutepixels(Bitmap *bp, uint16_t seed);
static void     unpermutepixels(Bitmap *bp, uint16_t seed);
static uint8_t  generatepixel(const uint8_t *coeff, int degree, int value);
static void     findclosestpair(int value, uint32_t *v1, uint32_t *v2);
static Bitmap   *newshadow(uint32_t width, uint32_t height, uint16_t seed, uint16_t shadownumber);
static Bitmap   **formshadows(Bitmap *bp, uint16_t seed, int k, int n);
static void     findcoefficients(int **mat, uint16_t k);
static void     revealsecret(Bitmap **shadows, uint16_t k, uint32_t width, uint32_t height, const char *filename);
static void     hideshadow(Bitmap *bp, Bitmap *shadow);
static Bitmap   *retrieveshadow(Bitmap *bp, uint32_t width, uint32_t height, uint16_t k);
static int      isvalidbmp(const char *filename, uint16_t k);
static int      isvalidshadow(const char *filename, uint16_t k);
static void     getvalidfilenames(char **filenames, char *dir, uint16_t k, uint16_t n, int (*isvalid)(const char *, uint16_t));
static void     getbmpfilenames(char **filenames, char *dir, uint16_t k, uint16_t n);
static void     getshadowfilenames(char **filenames, char *dir, uint16_t k);
static void     distributeimage(uint16_t k, uint16_t n, uint16_t seed, char *imgpath, char *dir);
static void     recoverimage(uint16_t k, uint32_t width, uint32_t height, char *filename, char *dir);
static int      countfiles(const char *dirname);

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
	215, 209, 50, 188, 167, 125, 250
};

int
isbigendian(void){
	int value = 1;

	return *(char *)&value != 1;
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

void
usage(void){
	die("usage: %s -(d|r) -secret image -k number -w width -h height -s seed"
			"[-n number] [-dir directory]\n", argv0);
}

void
swap(uint8_t *s, uint8_t *t){
	uint8_t temp;

	temp = *s;
	*s = *t;
	*t = temp;
}

int
mod(int a, int b){
	int m = a % b;

	return m < 0 ? m + b : m;
}

double 
randnormalize(void){
	return rand()/((double)RAND_MAX + 1);
}

long
randint(long int max){
	return (long) (randnormalize()*(max+1)); /*returns num in [0, max]*/
} 

uint32_t
get32bitsfromheader(FILE *fp, int offset){
	uint32_t value;
	uint32_t pos = ftell(fp);

	fseek(fp, offset, SEEK_SET);
	xfread(&value, sizeof(value), 1, fp);
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
newbitmap(uint32_t width, uint32_t height){
	int totalpixels = width * height;
	Bitmap *bmp     = xmalloc(sizeof(*bmp));
	bmp->imgpixels  = xmalloc(totalpixels);
	initpalette(bmp->palette);

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

	return bmp;
}

void
freebitmap(Bitmap *bp){
	free(bp->imgpixels);
	free(bp);
}

void
changeheaderendianness(BMPheader *h){
	uint32swap(&h->size);
	uint16swap(&h->unused1);
	uint16swap(&h->unused2);
	uint32swap(&h->offset);
}

void
changedibendianness(DIBheader *h){
	uint32swap(&h->size);
	uint32swap(&h->width);
	uint32swap(&h->height);
	uint16swap(&h->nplanes);
	uint16swap(&h->depth);
	uint32swap(&h->compression);
	uint32swap(&h->pixelarraysize);
	uint32swap(&h->hres);
	uint32swap(&h->vres);
	uint32swap(&h->ncolors);
	uint32swap(&h->nimpcolors);
}

void
readbmpheader(Bitmap *bp, FILE *fp){
	BMPheader *h = &bp->bmpheader;

	xfread(h->id, sizeof(h->id), 1, fp);
	xfread(&h->size, sizeof(h->size), 1, fp);
	xfread(&h->unused1, sizeof(h->unused1), 1, fp);
	xfread(&h->unused2, sizeof(h->unused2), 1, fp);
	xfread(&h->offset, sizeof(h->offset), 1, fp);

	if(isbigendian())
		changeheaderendianness(&bp->bmpheader);
}

void
writebmpheader(Bitmap *bp, FILE *fp){
	BMPheader h = bp->bmpheader;

	if(isbigendian())
		changeheaderendianness(&h);

	xfwrite(h.id, sizeof(h.id), 1, fp);
	xfwrite(&(h.size), sizeof(h.size), 1, fp);
	xfwrite(&(h.unused1), sizeof(h.unused1), 1, fp);
	xfwrite(&(h.unused2), sizeof(h.unused2), 1, fp);
	xfwrite(&(h.offset), sizeof(h.offset), 1, fp);
}

void
readdibheader(Bitmap *bp, FILE *fp){
	DIBheader *h = &bp->dibheader;

	xfread(&h->size, sizeof(h->size), 1, fp);
	xfread(&h->width, sizeof(h->width), 1, fp);
	xfread(&h->height, sizeof(h->height), 1, fp);
	xfread(&h->nplanes, sizeof(h->nplanes), 1, fp);
	xfread(&h->depth, sizeof(h->depth), 1, fp);
	xfread(&h->compression, sizeof(h->compression), 1, fp);
	xfread(&h->pixelarraysize, sizeof(h->pixelarraysize), 1, fp);
	xfread(&h->hres, sizeof(h->hres), 1, fp);
	xfread(&h->vres, sizeof(h->vres), 1, fp);
	xfread(&h->ncolors, sizeof(h->ncolors), 1, fp);
	xfread(&h->nimpcolors, sizeof(h->nimpcolors), 1, fp);

	if(isbigendian())
		changedibendianness(&bp->dibheader);
}

void
writedibheader(Bitmap *bp, FILE *fp){
	DIBheader h = bp->dibheader;

	if(isbigendian())
		changedibendianness(&h);

	xfwrite(&(h.size), sizeof(h.size), 1, fp);
	xfwrite(&(h.width), sizeof(h.width), 1, fp);
	xfwrite(&(h.height), sizeof(h.height), 1, fp);
	xfwrite(&(h.nplanes), sizeof(h.nplanes), 1, fp);
	xfwrite(&(h.depth), sizeof(h.depth), 1, fp);
	xfwrite(&(h.compression), sizeof(h.compression), 1, fp);
	xfwrite(&(h.pixelarraysize), sizeof(h.pixelarraysize), 1, fp);
	xfwrite(&(h.hres), sizeof(h.hres), 1, fp);
	xfwrite(&(h.vres), sizeof(h.vres), 1, fp);
	xfwrite(&(h.ncolors), sizeof(h.ncolors), 1, fp);
	xfwrite(&(h.nimpcolors), sizeof(h.nimpcolors), 1, fp);
}

Bitmap *
bmpfromfile(const char *filename){
	FILE *fp = xfopen(filename, "r");
	Bitmap *bp = xmalloc(sizeof(*bp));

	readbmpheader(bp, fp);
	readdibheader(bp, fp);
	xfread(bp->palette, sizeof(bp->palette), 1, fp);

	/* read pixel data */
	int imagesize = bmpimagesize(bp);
	bp->imgpixels = xmalloc(imagesize);
	xfread(bp->imgpixels, sizeof(bp->imgpixels[0]), imagesize, fp);
	xfclose(fp);

	return bp;
}

int
validate_k(FILE *fp, int r){
	int width  = bmpfilewidth(fp);
	int height = bmpfileheight(fp);
	int pixels = width * height;
	int aux    = pixels / r;

	return pixels == aux * r;
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
	int imgsize = bmpimagesize(bp);

	for(int i = 0; i < imgsize; i++)
		if(bp->imgpixels[i] > 250)
			bp->imgpixels[i] = 250;
}

void
permutepixels(Bitmap *bp, uint16_t seed){
	int i, j;
	int imgsize = bmpimagesize(bp);
	uint8_t *p  = bp->imgpixels;

	srand(seed);
	for(i = imgsize - 1; i > 1; i--){
		j = randint(i);
		swap(&p[j], &p[i]);
	}
}

void
unpermutepixels(Bitmap *bp, uint16_t seed){
	int i, j;
	int imgsize  = bmpimagesize(bp);
	int *permseq = xmalloc(sizeof(*permseq) * imgsize);
	uint8_t *p   = bp->imgpixels;

	srand(seed);
	for(i = imgsize - 1; i > 0; i--)
		permseq[i] = randint(i);

	for(i = 1 ; i < imgsize - 1; i++){
		j = permseq[i];
		swap(&p[j], &p[i]);
	}
	free(permseq);
}

/* uses coeff[0] to coeff[degree] to evaluate the corresponding
 * section polynomial and generate a pixel for a shadow image */
uint8_t
generatepixel(const uint8_t *coeff, int degree, int value){
	long ret = 0;

	for(int i = 0; i <= degree; i++)
		ret += coeff[i] * powl(value, i);

	return ret % PRIME;
}

/* find closest pair of values that when multiplied, give x */
void
findclosestpair(int x, uint32_t *width, uint32_t *height){
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
newshadow(uint32_t width, uint32_t height, uint16_t seed, uint16_t shadownumber){
	Bitmap *bmp            = newbitmap(width, height);
	bmp->bmpheader.unused1 = seed;
	bmp->bmpheader.unused2 = shadownumber;

	return bmp;
}

Bitmap **
formshadows(Bitmap *bp, uint16_t seed, int k, int n){
	uint8_t *coeff;
	uint32_t width, height;
	int i, j, totalpixels  = bmpimagesize(bp);;
	Bitmap **shadows = xmalloc(sizeof(*shadows) * n);

	findclosestpair(totalpixels/k, &width, &height);

	for(i = 0; i < n; i++)
		shadows[i] = newshadow(width, height, seed, i+1);

	/* generate shadow image pixels */
	for(j = 0; j*k < totalpixels; j++){
		coeff = &bp->imgpixels[j*k]; 
		for(i = 0; i < n; i++){
			shadows[i]->imgpixels[j] = generatepixel(coeff, k-1, i+1);
		}
	}

	return shadows;
}

void
findcoefficients(int **mat, uint16_t k){
	int i, j, t, a, temp;

	/* take matrix to echelon form */
	for(j = 0; j < k-1; j++){
		for(i = k-1; i > j; i--){
			a = mat[i][j] * modinv[mat[i-1][j]];
			for(t = j; t < k+1; t++){
				temp = mat[i][t] - ((mat[i-1][t] * a) % PRIME);
				mat[i][t] = mod(temp, PRIME);
			}
		}
	}

	/* take matrix to reduced row echelon form */
	for(i = k-1; i > 0; i--){
		mat[i][k] = (mat[i][k] * modinv[mat[i][i]]) % PRIME;
		mat[i][i] = (mat[i][i] * modinv[mat[i][i]]) % PRIME;
		for(t = i-1; t >= 0; t--){
			temp = mat[t][k] - ((mat[i][k] * mat[t][i]) % PRIME);
			mat[t][k] = mod(temp, PRIME);
			mat[t][i] = 0;
		}
	}
}

void
revealsecret(Bitmap **shadows, uint16_t k, uint32_t width, uint32_t height, const char *filename){
	int i, j, t, value;
	int pixels = (*shadows)->dibheader.pixelarraysize;
	Bitmap *bmp = newbitmap(width, height);
	Bitmap *sp;

	int **mat = xmalloc(sizeof(*mat) * k);
	for(i = 0; i < k; i++)
		mat[i] = xmalloc(sizeof(**mat) * (k+1));

	for(i = 0; i < pixels; i++){
		for(j = 0; j < k; j++){
			sp = shadows[j];
			value = sp->bmpheader.unused2;
			mat[j][0] = 1;
			for(t = 1; t < k; t++){
				mat[j][t] = value;
				value *= sp->bmpheader.unused2;
			}
			mat[j][k] = sp->imgpixels[i];
		}
		findcoefficients(mat, k);
		for(j = i * k; j < (i+1) * k; j++){
			bmp->imgpixels[j] = mat[j % k][k];
		}
	} 
	/* unpermutepixels(bmp, sp->bmpheader.unused1); */
	bmptofile(bmp, filename);

	for(i = 0; i < k; i++)
		free(mat[i]);
	free(mat);
	freebitmap(bmp);
}

void
hideshadow(Bitmap *bp, Bitmap *shadow){
	int i, j;
	uint8_t byte;
	char shadowfilename[20] = {0};
	int pixels = bmpimagesize(shadow);

	bp->bmpheader.unused1 = shadow->bmpheader.unused1;
	bp->bmpheader.unused2 = shadow->bmpheader.unused2;
	snprintf(shadowfilename, 20, "shadow%d.bmp", shadow->bmpheader.unused2);

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
retrieveshadow(Bitmap *bp, uint32_t width, uint32_t height, uint16_t k){
	uint8_t byte, mask;
	uint16_t key          = bp->bmpheader.unused1;
	uint16_t shadownumber = bp->bmpheader.unused2;

	if(bp->dibheader.width * bp->dibheader.height < (uint32_t)(((width * height)*8)/k))
		die("image of %d pixels can't contain shadow of %d pixels\n",
				bp->dibheader.width * bp->dibheader.height, ((width * height)*8)/k);

	findclosestpair((width * height)/k, &width, &height);
	Bitmap *shadow = newshadow(width, height, key, shadownumber);
	int shadowpixels = shadow->dibheader.pixelarraysize;

	for(int i = 0; i < shadowpixels; i++){
		byte = 0;
		mask = 0x80; /* 1000 0000 */
		for(int j = i*8; j < 8*(i+1); j++){
			if(bp->imgpixels[j] & 0x01)
				byte |= mask;
			mask >>= 1;
		}
		shadow->imgpixels[i] = byte;
	}

	return shadow;
}

int
isvalidbmp(const char *filename, uint16_t k){
	uint16_t magicnumber;
	FILE *fp  = xfopen(filename, "r");
	int valid = 0;

	xfread(&magicnumber, sizeof(magicnumber), 1, fp);
	if(!isbigendian())
		uint16swap(&magicnumber);
	if(magicnumber == BMP_MAGIC_NUMBER)
		valid = validate_k(fp, k);
	xfclose(fp);

	return valid;
}

int
isvalidshadow(const char *filename, uint16_t k){
	uint16_t shadownumber;
	FILE *fp = xfopen(filename, "r");

	fseek(fp, 8, SEEK_SET);
	xfread(&shadownumber, sizeof(shadownumber), 1, fp);
	xfclose(fp);

	return shadownumber && isvalidbmp(filename, k);
}

void 
getvalidfilenames(char **filenames, char *dir, uint16_t k, uint16_t n, int (*isvalid)(const char *, uint16_t)){
	struct dirent *d;
	DIR *dp = xopendir(dir);
	char filepath[255] = {0};
	int i = 0;

	while((d = readdir(dp)) && i < n){
		if(d->d_type == DT_REG){ 
			snprintf(filepath, 255, "%s/%s", dir, d->d_name);
			if(isvalid(filepath, k)){
				filenames[i] = xmalloc(strlen(filepath) + 1);
				strcpy(filenames[i], filepath);
				i++;
			}
		}
	}
	closedir(dp);

	if(i < n)
		die("not enough valid bmps for a (%d,%d) threshold in dir %s", k, n, dir);
}

void
getbmpfilenames(char **filenames, char *dir, uint16_t k, uint16_t n){
	getvalidfilenames(filenames, dir, k, n, isvalidbmp);
}

void
getshadowfilenames(char **filenames, char *dir, uint16_t k){
	getvalidfilenames(filenames, dir, k, k, isvalidshadow);
}

void
distributeimage(uint16_t k, uint16_t n, uint16_t seed, char *imgpath, char *dir){
	char **filepaths = xmalloc(sizeof(*filepaths) * n);
	Bitmap *bp, **shadows;
	int i;

	getbmpfilenames(filepaths, dir, k, n);
	bp = bmpfromfile(imgpath);
	truncategrayscale(bp);
	/* permutepixels(bp, seed); */
	shadows = formshadows(bp, seed, k, n);
	freebitmap(bp);

	for(i = 0; i < n; i++){
		bp = bmpfromfile(filepaths[i]);
		hideshadow(bp, shadows[i]);
		freebitmap(bp);
	}

	for(i = 0; i < n; i++){
		free(filepaths[i]);
		freebitmap(shadows[i]);
	}
	free(filepaths);
	free(shadows);
}

void
recoverimage(uint16_t k, uint32_t width, uint32_t height, char *filename, char *dir){
	char **filepaths = xmalloc(sizeof(*filepaths) * k);
	Bitmap **shadows = xmalloc(sizeof(*shadows) * k);
	Bitmap *bp;
	int i;

	getshadowfilenames(filepaths, dir, k);
	for(i = 0; i < k; i++){
		bp = bmpfromfile(filepaths[i]);
		shadows[i] = retrieveshadow(bp, width, height, k);
		freebitmap(bp);
	}
	revealsecret(shadows, k, width, height, filename);

	for(i = 0; i < k; i++){
		free(filepaths[i]);
		freebitmap(shadows[i]);
	}
	free(filepaths);
	free(shadows);
}

int
countfiles(const char *dirname){
	struct dirent *d;
	int filecount = 0;
	DIR *dp = xopendir(dirname);

	while((d = readdir(dp)))
		if(d->d_type == DT_REG) /* If the entry is a regular file */
			filecount++;				
	closedir(dp);

	return filecount;
}

int
main(int argc, char *argv[]){
	char *filename, *dir;
	int dflag = 0;
	int rflag = 0;
	int kflag = 0;
	int wflag = 0;
	int hflag = 0;
	int sflag = 0;
	int nflag = 0;
	int dirflag = 0;
	int secretflag = 0;
	uint16_t seed, k, n;;
	int width, height;
	int i;

	argv0 = argv[0]; /* save program name for usage() */

	for(i = 1; i < argc; i++){
		if(strcmp(argv[i], "-d") == 0){
			dflag = 1;
		} else if(strcmp(argv[i], "-r") == 0) {
			rflag = 1;
		} else if(strcmp(argv[i], "-secret") == 0){
			secretflag = 1;
			if(i + 1 <= argc){
				filename = argv[++i];
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-k") == 0){
			kflag = 1;
			if(i + 1 <= argc){
				k = atoi(argv[++i]);
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-w") == 0){
			wflag = 1;
			if(i + 1 <= argc){
				width = atoi(argv[++i]);
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-h") == 0){
			hflag = 1;
			if(i + 1 <= argc){
				height = atoi(argv[++i]);
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-s") == 0) {
			sflag = 1;
			if(i + 1 <= argc){
				seed = atoi(argv[++i]);
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-n") == 0){
			nflag = 1;
			if(i + 1 <= argc){
				n = atoi(argv[++i]);
			} else {
				usage();
			}
		} else if(strcmp(argv[i], "-dir") == 0){
			dirflag = 1;
			if(i + 1 <= argc){
				dir = argv[++i];
			} else{
				usage();
			}
		} else {
			die("invalid %s parameter \n", argv[i]);
		}
	}

	if(!(dflag || rflag) || !secretflag || !kflag)
		usage();
	if((rflag && !(wflag && hflag)) || !width || !height)
		die("specify a positive width and height with -w -h for the revealed image\n");

	if(!dirflag)
		dir = "./";
	if(!nflag)
		n = countfiles(dir);
	if(!sflag)
		seed = DEFAULT_SEED;

	if(k > n || k < 2 || n < 2)
		die("k and n must be: 2 <= k <= n\n");
	if(dflag && rflag)
		die("can't use -d and -r flags simultaneously\n");
	//printf("%s\n",dir);
	//printf("%d\n",dirflag);
	if(dflag)
		distributeimage(k, n, seed, filename, dir);
	else if(rflag)
		recoverimage(k, width, height, filename, dir);

	return EXIT_SUCCESS;
}
