#include <stdlib.h>
#include <stdio.h>

#ifndef UNICODE
#define UNICODE
#endif
//#include <limits.h> //-D_GNU_SOURCE necessario in compilazione per aver IOV_MAX

//#include <string.h>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

//compilare con -lws2_32

#define MAX_QUEUE 2 //numero massimo di richieste di connessioni permesse in un dato momento

typedef struct thread_info {  //struttura per passare parametri al thread
	SOCKET AcceptSocket;
	SOCKADDR_IN peer_addr;
	int thread_number;
} thr_info;

void print_addr(SOCKADDR_IN *addr){
	//printf("\nInfo:\n\n");
	//printf("FAMILY: %d\n", addr->sin_family);
	printf("ADDR: %s \nPORT: %d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
	putc('\n',stdout);
}

SOCKET make_socket(char* port){

	SOCKET ListenSocket = INVALID_SOCKET;

	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if(ListenSocket == INVALID_SOCKET){
		wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
		WSACleanup();
		return ListenSocket;
	} 

	SOCKADDR_IN service;

	service.sin_family = AF_INET;
    service.sin_addr.s_addr = htonl(INADDR_ANY);
    service.sin_port = htons(atoi(port));


    int iResult = bind(ListenSocket, (SOCKADDR *) &service, sizeof(service));
	
	if (iResult == SOCKET_ERROR) {
        wprintf(L"bind function failed with error %d\n", WSAGetLastError());
        iResult = closesocket(ListenSocket);
        if (iResult == SOCKET_ERROR)
            wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return ListenSocket;
    }
    
    print_addr(&service);

    return ListenSocket;
}

void connecting(void *param){

	SOCKET *AcceptSocket = &(((thr_info *)param)->AcceptSocket);
	SOCKADDR_IN *peer_addr = &(((thr_info *)param)->peer_addr);
	int *connection_id = &(((thr_info *)param)->thread_number);

    print_addr(peer_addr);

    char buffer[1024];
    char *pointer;

    int data_size = 0;

    do{

		data_size = recv(*AcceptSocket, buffer, 1024, 0);

		if(data_size == SOCKET_ERROR){
			if(WSAGetLastError() == 10054) wprintf(L"Client disconnesso\n\n");
			closesocket(*AcceptSocket);
			free(param);
        	return;	
		}
		else if(data_size == 0) continue;

		printf("dati ricevuti: %d Byte\n\n",data_size);

		pointer = buffer;
		printf("-----------------------------------------\n");
		for(int i = 0; i < data_size; i++, pointer++){
			putc(*pointer, stdout);
		}
		printf("-----------------------------------------\n\n");

	}while(data_size);

	closesocket(*AcceptSocket);
	free(param);
}

int main(int argc, char* argv[]){

	if(argc < 2){
		printf("Usare: %s [port]\n",argv[0]);
		return 0;
	}

	WSADATA wsaData = {0};
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (iResult != 0) {
        wprintf(L"WSAStartup failed: %d\n", iResult);
        return 1;
    }

	/*MAKE SOCKET AND BINDING---------------------------*/
	SOCKET ListenSocket = make_socket(argv[1]);

	if(ListenSocket == SOCKET_ERROR){
		wprintf(L"socket function failed with error = %d\n", WSAGetLastError());
		return -1;
	}
    
    /*LISTENING-----------------------------------------*/
	iResult = listen(ListenSocket, MAX_QUEUE);  //sockfd è il descrittore del socket, mentre MAX_QUEUE è il numero massimo di connessioni ascoltabili in un dato momento
	int connessioni = 0;

	if(iResult == SOCKET_ERROR){
		wprintf(L"listen function failed with error: %d\n", WSAGetLastError());
	}

	wprintf(L"Ascolto...\n");

	char v = 0;
	int peer_addrlen = sizeof(SOCKADDR_IN);

    while(v == 0){
    	connessioni++;

    	thr_info *param = malloc(sizeof(thr_info));  //preparo i parametri utilizzati per l'accettazione e utilizzati nel thread
		param->thread_number = connessioni;
		memset(&(param->peer_addr), 0, peer_addrlen);

    	param->AcceptSocket = accept(ListenSocket, (SOCKADDR *)&(param->peer_addr), &peer_addrlen);  //al ritorno dalla funzione, riceveremo l'indirizzo del peer in peer_addr

    	if (param->AcceptSocket == INVALID_SOCKET) {
        	wprintf(L"accept failed with error: %ld\n", WSAGetLastError());
        	closesocket(ListenSocket);
        	WSACleanup();
        	return -1;
    	} else{
    		wprintf(L"\nNew Client connected:\n");
 		}

    	HANDLE hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)connecting, (void *) param, 0, 0);
    	//v = accepting(ListenSocket);

    	//senza thread dobbiamo disconnetterci dalla connessione 
    	if (hThread == NULL){
			printf("cannot create thread for incoming connection!\n");
			closesocket(param->AcceptSocket);
			free(param);
		}
    }

	iResult = closesocket(ListenSocket);
    if (iResult == SOCKET_ERROR) {
        wprintf(L"closesocket function failed with error %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

	WSACleanup();
	return 0;

	//per poter connettersi da fuori il pc, è necessario fare un forwarding dall'ip dell'host a quello del wsl
	/*netsh interface portproxy add v4tov4 listenport=<yourPortToForward> listenaddress=0.0.0.0 connectport=<yourPortToConnectToInWSL> connectaddress=(wsl hostname -I)*/
}
