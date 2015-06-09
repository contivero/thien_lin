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
#define PALETTE_SIZE           1024
#define PIXEL_ARRAY_OFFSET     BMP_HEADER_SIZE + DIB_HEADER_SIZE + PALETTE_SIZE
#define FILESIZE_OFFSET        2	
#define WIDTH_OFFSET           18
#define HEIGHT_OFFSET          22
#define PRIME                  251

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
	BMPheader bmpheader;  /* 14 bytes bmp starting header */
	DIBheader dibheader;  /* 40 bytes dib header */
	uint8_t *palette;     /* color palette */
	uint8_t *imgpixels;   /* array of bytes representing each pixel */
} Bitmap;

/* prototypes */ 
static void     die(const char *errstr, ...);
static void     *xmalloc(size_t size);
static FILE     *xfopen(const char *filename, const char *mode);
static void     xfclose(FILE *fp);
static void     xfread(void *ptr, size_t size, size_t nmemb, FILE *stream);
static void     xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream);
static void     usage(void);
static long     randint(long int max);
static double   randnormalize(void);
static int      generatepixel(const uint8_t *coeff, int degree, int value);
static uint32_t get32bitsfromheader(FILE *fp, int offset);
static uint32_t bmpfilesize(FILE *fp);
static uint32_t bmpfilewidth(FILE *fp);
static uint32_t bmpfileheight(FILE *fp);
static uint32_t bmpfiledibheadersize(FILE *fp);
static void     freebitmap(Bitmap *bp);
static Bitmap   *loadbmp(const char *filename, int r);
static Bitmap   *bmpfromfile(FILE *fp);
static void     bmptofile(Bitmap *bp, const char *filename);
static int      bmpimagesize(Bitmap *bp);
static int      bmppalettesize(Bitmap *bp);
static Bitmap   **formshadows(Bitmap *bp, int r, int n);

/* globals */
static char *argv0; /* program name for usage() */
static uint16_t swidth;	/* shadow width */
static uint16_t sheight; /* shadow width */
static int modular_inverse[PRIME] = { /* modular multiplicative inverse */
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
		die("couldn't open %s", filename);

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
		die("read error"); /* TODO print errno string! */
}

void 
xfwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream){
	if (fwrite(ptr, size, nmemb, stream) != nmemb)
		die("Error in writing or end of file.\n");
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
get32bitsfromheader(FILE *fp, int offset){
	uint32_t value;
	uint32_t pos = ftell(fp);

	fseek(fp, offset, SEEK_SET);
	xfread(&value, 4, 1, fp);
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
	int palettesize = bmppalettesize(bp);
	bp->palette = xmalloc(palettesize);
	xfread(bp->palette, 1, palettesize, fp);

	/* read pixel data */
	int imagesize = bmpimagesize(bp);
	bp->imgpixels = xmalloc(imagesize);
	xfread(bp->imgpixels, 1, imagesize, fp);

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

	writebmpheader(bp, fp);
	writedibheader(bp, fp);
	xfwrite(bp->palette, bmppalettesize(bp), 1, fp);
	xfwrite(bp->imgpixels, bmpimagesize(bp), 1, fp);
	xfclose(fp);
}

void
truncategrayscale(Bitmap *bp){
	int palettesize = bmppalettesize(bp);

	for(int i = 0; i < palettesize; i++)
		if(bp->palette[i] > 250)
			bp->palette[i] = 250;
}

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

/* uses coeff[0] to coeff[degree] to evaluate the corresponding
 * section polynomial and generate a pixel for a shadow image */
int
generatepixel(const uint8_t *coeff, int degree, int value){
	long ret = 0;

	for(int i = 0; i <= degree; i++)
		ret += coeff[i] * powl(value,i);

	return ret % PRIME;
}

Bitmap *
newshadow(Bitmap *bp, int r, int shadownumber){
	int width = bp->dibheader.width;
	int height = bp->dibheader.height;
	int totalpixels = (width * height)/r;

	Bitmap *sp    = xmalloc(sizeof(*sp));
	sp->palette   = xmalloc(PALETTE_SIZE);
	sp->imgpixels = xmalloc(totalpixels);

	memcpy(sp->palette, bp->palette, PALETTE_SIZE);
	memcpy(&sp->bmpheader, &bp->bmpheader, sizeof(bp->bmpheader));
	memcpy(&sp->dibheader, &bp->dibheader, sizeof(bp->dibheader));

	sp->dibheader.height = sheight;
	sp->dibheader.width = swidth;
	sp->dibheader.pixelarraysize = totalpixels;
	sp->bmpheader.size = totalpixels + PIXEL_ARRAY_OFFSET;
	sp->bmpheader.unused2 = shadownumber;

	return sp;
}

/* find closest pair of numbers that when multiplied, give the input.
 * both of them are stored in the swidth and sheight globals */
void
findwidthheight(int x){
	int i;
	int n = floor(sqrt(x));
	swidth = 1;

	for(;;){
		if(x % n == 0){
			swidth *= n;
			sheight = x / n;
			break;
		} else {
			n--;
		}
	}
	printf("swidth = %d, sheight = %d", swidth, sheight);
}

Bitmap **
formshadows(Bitmap *bp, int r, int n){
	uint8_t *coeff;
	int i, j;
	int totalpixels = bmpimagesize(bp);
	Bitmap **shadows = xmalloc(sizeof(*shadows) * n);

	findwidthheight(totalpixels/r);

	/* allocate memory for shadows and copy necessary data */
	for(i = 0; i < n; i++)
		shadows[i] = newshadow(bp, r, i+1);

	/* generate shadow image pixels */
	for(j = 0; j*r < totalpixels; j++){
		for(i = 0; i < n; i++){
			coeff = &bp->imgpixels[j*r]; 
			shadows[i]->imgpixels[j] = generatepixel(coeff, r-1, i+1);
		}
	}

	return shadows;
}

Bitmap *
recover(Bitmap **shadows, int r){
	int i, j;
	int npixels = bmpimagesize(*shadows);
	Bitmap *bp = xmalloc(sizeof(*bp));

	for(i = 0; i < npixels; i++){
		for(j = 0; j < r; j++){
			// TODO
		}
	}

	return bp;
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
revealpixels(int **mat, int r){
	uint8_t *pixels = xmalloc(sizeof(*pixels) * r);
	int i, j, k;
	int a;

	/* take matrix to echelon form */
	for(j = 0; j < r-1; j++){
		for(i = r-1; i > j; i--){
			a = mat[i][j] * modular_inverse[mat[i-1][j]];
			for(k = j; k < r+1; k++){
				mat[i][k] -= (mat[i-1][k] * a) % PRIME;
			}
		}
	}

	/* take matrix to reduced row echelon form */
	for(i = r-1; i > 0; i--){
		mat[i][r] = (mat[i][r] * modular_inverse[mat[i][i]]) % PRIME;
		mat[i][i] = 1;
		for(k = i-1; k >= 0; k--){
			mat[k][r] = (mat[k][r] - ((mat[i][r] * mat[k][i]) % PRIME)) % PRIME;
			mat[k][i] = 0;
		}
	}
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

void
reveal(Bitmap **shadows, int r){
	int i, j, k, t, value;
	uint8_t *pixels;
	Bitmap *sp;
	int **mat = xmalloc(sizeof(*mat) * r);

	for(i = 0; i < r; i++){
		mat[i] = xmalloc(sizeof(**mat) * (r+1));
	}

	/* TODO ver si puedo usar la función newshadow para esto */
	sp = shadows[0];
	Bitmap *bmp = xmalloc(sizeof(*bmp));
	bmp->palette = xmalloc(bmppalettesize(sp));
	memcpy(bmp->palette, sp->palette, bmppalettesize(sp));
	bmp->imgpixels = xmalloc(sp->dibheader.pixelarraysize * r);
	memcpy(&bmp->bmpheader, &sp->bmpheader, sizeof(sp->bmpheader));
	memcpy(&bmp->dibheader, &sp->dibheader, sizeof(sp->dibheader));
	bmp->dibheader.height = 512; /* TODO NO CABLEAR, AVERIGUAR DE DONDE SACAR */
	bmp->dibheader.width = 512; /* TODO NO CABLEAR, AVERIGUAR DE DONDE SACAR */
	bmp->dibheader.pixelarraysize = sp->dibheader.pixelarraysize * r;
	bmp->bmpheader.size = sp->dibheader.pixelarraysize*r + PIXEL_ARRAY_OFFSET;

	for(i = 0; i < (*shadows)->dibheader.pixelarraysize; i++){
		for(j = 0; j < r; j++){
			sp = shadows[j];
			value = sp->bmpheader.unused2;
			mat[j][0] = 1;
			for(k = 1; k < r; k++){
				mat[j][k] = value;
				value *= value;
			}
			mat[j][r] = sp->imgpixels[i];
		}
		revealpixels(mat, r);
		for(t = i * r; t < (i+1) * r; t++){
			bmp->imgpixels[t] = mat[t % r][r];
		}
	}
//	permutepixels(bmp);
	bmptofile(bmp, "revealed.bmp");
}

int
main(int argc, char *argv[]){
	char *filename;

	/* keep program name for usage() */
	argv0 = argv[0]; 

	int r = 2, n = 4; /* TODO receive as parameter and parse */

	filename = argv[1];
	Bitmap *bp = loadbmp(filename, r);
	truncategrayscale(bp);
//	permutepixels(bp);
	Bitmap **shadows = formshadows(bp, r, n);

	reveal(shadows, r);

	/* /1* write shadows to disk *1/ */
	/* for(n -= 1; n >= 0; n--){ */
	/* 	snprintf(filename, 256, "shadow%d.bmp", n); */
	/* 	printf("%s\n", filename); */
	/* 	bmptofile(shadows[n], filename); */
	/* 	freebitmap(shadows[n]); */ 
	/* } */
	/* freebitmap(bp); */
	/* free(shadows); */
}
