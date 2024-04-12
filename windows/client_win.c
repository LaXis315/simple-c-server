#include <stdlib.h>
#include <stdio.h>

#ifndef UNICODE
#define UNICODE
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h> //compilare con -lws2_32

void print_addr(SOCKADDR_IN *addr){
	printf("\n\nInfo:\n\n");
	printf("FAMILY: %d\n", addr->sin_family);
	printf("ADDR: %s \nPORT: %d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
	putc('\n',stdout);
}

SOCKET make_socket(){

	SOCKET ListenSocket = INVALID_SOCKET;

	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ListenSocket == INVALID_SOCKET){
		wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
		WSACleanup();
		return ListenSocket;
	}

	return ListenSocket;
	
}

int main(int argc, char* argv[]){

    if(argc < 3){
        printf("Usare: '%s [host] [port]'\n", argv[0]);
        return -1;
    }

    WSADATA wsaData = {0};
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
        wprintf(L"WSAStartup failed: %d\n", iResult);
        return 1;
    }

	SOCKET ListenSocket = make_socket();

	if(ListenSocket == INVALID_SOCKET) return -1;
//----------------------------
	SOCKADDR_IN destination;

	destination.sin_family = AF_INET;
    if(inet_pton(AF_INET, argv[1], &(destination.sin_addr.s_addr)) == -1){
    	printf("Error %d in converting IP address\n", WSAGetLastError());
	}

    destination.sin_port = htons(atoi(argv[2]));

    if(destination.sin_port == 0){
    	printf("Port not valid\n");
    	return -1;
    }

	print_addr(&destination);

    if(connect(ListenSocket, (SOCKADDR *)&destination, sizeof(destination)) == -1){
        printf("Errore connessione\n");
        return -1;
    }

    printf("\nConnesso!\n");
	printf("Scrivi un messaggio:\n\n");

	char buffer[1024];  //buffer momentaneo per i messaggi prima che sia allocata memoria

	int BufferCount = 0;//numero di iovec da mandare
	WSABUF * Buffers= malloc(0);
	char **strings = malloc(0);


	while(fgets(buffer, 1023, stdin) != NULL && *buffer != '\n'){  //raccolgo le stringhe e le metto nei vari blocchi
		BufferCount++;
		Buffers = realloc(Buffers, sizeof(WSABUF)*BufferCount); //crea un nuovo iovec
		
		if(Buffers == NULL){ 
			printf("memoria non reallocata\n");
			return -1;
		}
		
		strings = realloc(strings, sizeof(char**)*BufferCount);
		
		if(strings == NULL){ 
			printf("memoria non reallocata\n");
			return -1;
		}
		
		size_t lenght = strlen(buffer)+1;
		strings[BufferCount-1] = malloc(lenght);
		if(strings[BufferCount-1] == NULL){ 
			printf("memoria non reallocata\n");
			return -1;
		}
		strncpy(strings[BufferCount-1], buffer, lenght);

		Buffers[BufferCount-1].len = lenght;
		Buffers[BufferCount-1].buf = strings[BufferCount-1]; //Buffer completo, passo al prossimo
	}

	for(int i = 0; i < BufferCount; i++){
		send(ListenSocket, Buffers[i].buf, Buffers[i].len, 0);
		free(strings[i]);
	}

	free(Buffers);
	free(strings);

	return 0;
    
}