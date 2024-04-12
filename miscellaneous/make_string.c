#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]){

	if(argc < 3){
		printf("Usare: '%s [carattere] [numero di caratteri]'\n",argv[0]);
	}

	int n = atoi(argv[2]);

	char c = *argv[1]; // prendo il primo carattere

	for(int i = 0; i < n; i++) putc(c,stdout);

}