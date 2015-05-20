#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define WIDTH_OFFSET 18
#define WIDTH_FIELD_SIZE 4
#define HEIGHT_OFFSET 22
#define HEIGHT_FIELD_SIZE 4
#define END_OF_HEADER 54

/* prototypes */ 
void randomize(int num);
double randnormalize(void);
long int randint(long int max);
uint32_t bmpwidth(FILE *fp);
uint32_t bmpheight(FILE *fp);

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
