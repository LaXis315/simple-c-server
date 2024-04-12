#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]){

	if(argc < 2) return 0;
	
	if(strcmp(argv[1], "1") == 0){
		char buffer[10];

		if(fgets(buffer, 9, stdin) == NULL){
			return -1;
		}

		char * c = strchr(buffer,'\n');
		if(c == NULL){
			printf("Stringa troppo lunga\n");
			return -1;
		}
		*c = '\0';

		printf("%s\n", buffer);

	}
	else if(strcmp(argv[1], "2") == 0){
		char buffer[10];

		while(fgets(buffer, 9, stdin) != NULL && *buffer != '\n'){
			printf("%s", buffer);
		}
		
	}

}