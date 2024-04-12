#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h> //-D_GNU_SOURCE necessario in compilazione per aver IOV_MAX

#include <string.h>


#include <unistd.h>		//per creare i thread necessari a gestire le varie richieste
#include <pthread.h>
#include <sys/socket.h>  //"man sys_socket.h" libreria per gestire i socket
#include <sys/uio.h>

#include <netinet/in.h>  //libreria per ottenere l'indirizzo ip della macchina da usare poi in socket.h
#include <arpa/inet.h> //per convertire da host-bit-order a network-bit-order
#include <netdb.h> //da utilizzare per ottenere facilmente interfacce del computer e creare sockaddr

#include <poll.h>  //libreria per il polling del file descriptor (attendiamo che ci sia una richiesta i/o)

#define MAX_QUEUE 10 //numero massimo di richieste di connessioni permesse in un dato momento

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

typedef struct thread_info {  //struttura per passare parametri al thread
	int sockfd;
	int thread_number;
} thr_info;

void print_addr(sockaddr *addr, int thread_number){
	printf("\n\nConnessione n.%d Info:\n\n", thread_number);
	printf("FAMILY: %d\n", addr->sa_family);
	sockaddr_in * addr_in = (sockaddr_in *) addr;
	printf("ADDR: %s \nPORT: %d\n", inet_ntoa(addr_in->sin_addr), ntohs(addr_in->sin_port));
}

int make_socket(char* port){

	struct addrinfo hints;
	struct addrinfo *result;
	int sockfd;

	memset(&hints, 0, sizeof(hints)); //resettiamo hints

	hints.ai_family = AF_INET;    /* Allow IPv4*/
    hints.ai_socktype = SOCK_STREAM; /* stream socket */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_protocol = 0;          /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
   	hints.ai_next = NULL;

   	int value = getaddrinfo(NULL, port, &hints, &result);  //qua stiamo indicando una porta specifica e indirizzo qualsiasi

   	if (value != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(value));
        exit(EXIT_FAILURE);
    }

	/*ORA ESEGUIAMO IL BINDING PER LA PRIMA STRUTTURA TROVATA*/

    sockfd = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (sockfd == -1){
    	freeaddrinfo(result);
        exit(-1);
    }

    errno = 0;
    if(bind(sockfd, result->ai_addr, result->ai_addrlen) == -1){
		if(errno == EACCES) printf("L'indirizzo con qui si sta eseguendo il binding e' protetto.\n");
		else if(errno == EADDRINUSE) printf("Indirizzo bindato gia in uso\n");
		else if(errno == EINVAL) printf("addrlen e'sbagliato oppure addr non e'un indirizzo valido\n");
		else if(errno == EADDRNOTAVAIL) printf("Interfaccia non esistente\n");
		else if(errno == ENAMETOOLONG) printf("address troppo lungo\n");
		fprintf(stderr, "Error binding\n");
		close(sockfd);
		freeaddrinfo(result);
		exit(-1);
	}
    
    print_addr(result->ai_addr, 0);
    freeaddrinfo(result);

    return sockfd;
}

void *accepting(void *params){

	int sockfd_conn;
    sockaddr_in peer_addr;
    socklen_t peer_addrlen = sizeof(peer_addr);

    memset(&peer_addr, 0, sizeof(peer_addr));

    sockfd_conn = accept(((thr_info *)params)->sockfd, (sockaddr *)&peer_addr, &peer_addrlen);  //al ritorno dalla funzione, riceveremo l'indirizzo del peer in peer_addr

    print_addr((sockaddr *)&peer_addr, ((thr_info *)params)->thread_number);

    /*POLLING-------------------------------------------*/

    //facciamo un polling anche per la connessione per leggere tutti i messaggi in arrivo

    ssize_t data_size = 0;

    errno = 0;
    
    char buffer[1024];
    char *pointer;

    do{

		data_size = read(sockfd_conn, buffer, 1024);

		if(data_size <= 0){
			close(sockfd_conn);
			free(params);
        	return NULL;	
		}

		printf("\n\n*****************************************\n");
		printf("%zu Byte ricevuti da connessione: %d\n\n",data_size, ((thr_info *)params)->thread_number);

		pointer = buffer;
		printf("-----------------------------------------\n");
		for(int i = 0; i < data_size; i++, pointer++){
			putc(*pointer, stdout);
		}
		printf("-----------------------------------------\n\n");

	}while(1);
	
}



int main(int argc, char* argv[]){

	if(argc < 2){
		printf("Usare: %s [port]\n",argv[0]);
		return 0;
	}

	/*MAKE SOCKET AND BINDING---------------------------*/
	int sockfd = (int)make_socket(argv[1]);
    
    /*LISTENING-----------------------------------------*/
    errno = 0;
	int value = listen(sockfd, MAX_QUEUE);  //sockfd è il descrittore del socket, mentre MAX_QUEUE è il numero massimo di connessioni ascoltabili in un dato momento

	int connessioni = 0;


	if(value == -1){
		if(errno == EADDRINUSE) printf("Errore, un'altro socket è in ascolto sulla porta scelta\n");
		return -1;
	}

/*POLLING------------------------------------------------*/

	struct pollfd fds;

	fds.fd = sockfd;
	fds.events = POLLIN; //ci aspettiamo dati di normali e di priorità senza essere bloccati

	printf("\n\nIn attesa di connessioni...\n\n");
	while(1){
		memset(&(fds.revents), 0, sizeof(fds.revents));
		errno = 0;
		value = poll(&fds, 1, -1); //infinite polling

		if(value == -1){
			if(errno == EINTR) printf("Segnale catturato durante il polling");
			return -1;
		}

		if((fds.revents & POLLHUP) || (fds.revents & POLLERR) || (fds.revents & POLLNVAL)){
			printf("Errore con il socket, disconnessione dal socket in corso...");
			return -2;
		}

		if(fds.revents & POLLIN){
			/*qua facciamo il thread che si occupa della lettura e accettazione */
			connessioni++;

			pthread_t thread_id;
			thr_info *param = malloc(sizeof(thr_info));
			param->sockfd = sockfd;
			param->thread_number = connessioni;
            pthread_create(&thread_id, NULL, accepting, (void *)param);

		}
	}

	//per poter connettersi da fuori il pc, è necessario fare un forwarding dall'ip dell'host a quello del wsl
	/*netsh interface portproxy add v4tov4 listenport=<yourPortToForward> listenaddress=0.0.0.0 connectport=<yourPortToConnectToInWSL> connectaddress=(wsl hostname -I)*/
}
