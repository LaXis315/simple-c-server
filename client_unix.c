#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#include <string.h>

#include <unistd.h>		//per creare i thread necessari a gestire le varie richieste
#include <sys/socket.h>  //"man sys_socket.h" libreria per gestire i socket

#include <netinet/in.h>  //libreria per ottenere l'indirizzo ip della macchina da usare poi in socket.h
#include <arpa/inet.h> //per convertire da host-bit-order a network-bit-order

#include <poll.h>  //libreria per il polling del file descriptor (attendiamo che ci sia una richiesta i/o)

#define BUFFER 500

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

void print_addr_in(sockaddr_in *addr){
	printf("FAMILY: %d\n", addr->sin_family);
	printf("ADDR: %s PORT: %d\n", inet_ntoa(addr->sin_addr), ntohs(addr->sin_port));
}

int file_descriptor(){
	// creiamo un socket con 
	// int socket(int domain, int type, int protocol);
	//
	// il dominio indica la famiglia di protocolli da utilizzare, queste sono definite in sys/socket.h e informazioni su di esse sono visibili con "man address_families"
	// il type indica invece la semantica della comunicazione
	// il protocol invece indica quale protocollo scegliere dal dominio. con "man 5 protocols" possiamo vedere il numero associato ad ogni protocollo di ogni dominio, con "man 3 getprotoent" invece per vedere come mappare la strigna con nome di un protocollo al suo numero
	

	int sockfd = socket(AF_INET,SOCK_STREAM,0); //con ipv4 e sock_stream il protocollo default è TCP

	if(sockfd == -1){
		if(errno == EACCES) printf("Permessi non sufficienti a creare il socket\n");
		else if(errno == EPROTONOSUPPORT) printf("Protocollo non valido per la famiglia specificata");
		return -1;
	}

	return sockfd;
}

int main(int argc, char* argv[]){

    if(argc < 3){
        printf("Usare: '%s [host] [port] <message>'\n", argv[0]);
        return -1;
    }

	int sockfd = file_descriptor();
	
	printf("sockfd: %d\n",sockfd);

	sockaddr *addr;
	sockaddr_in addr_in;

    addr_in.sin_family = AF_INET;
    
    int port = atoi(argv[2]);
    if(!port){
        printf("Porta non valida\n");
        return -1;
    }
    
    addr_in.sin_port = htons(port);
    inet_aton(argv[1],&(addr_in.sin_addr));

	addr = (sockaddr *) &addr_in;  //grazie a questo casting, le funzioni della libreria socket possono leggere addr_in tramite il puntatore addr

	print_addr_in(&addr_in);


    
    if(connect(sockfd, addr, sizeof(addr_in)) == -1){
        printf("Errore connessione\n");
        return -1;
    }

    printf("Connesso!\n");

    for(int i = 3; i < argc; i++){
        size_t len = strlen(argv[i]) +1;
        write(sockfd, argv[i], len);
        printf("Inviato: %s\n",argv[i]);
    }
    

    
}