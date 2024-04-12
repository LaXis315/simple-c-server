#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include <string.h>

#include <unistd.h>		//per creare i thread necessari a gestire le varie richieste
#include <sys/socket.h>  //"man sys_socket.h" libreria per gestire i socket

#include <netinet/in.h>  //libreria per ottenere l'indirizzo ip della macchina da usare poi in socket.h
#include <arpa/inet.h> //per convertire da host-bit-order a network-bit-order
#include <netdb.h> //da utilizzare per ottenere facilmente interfacce del computer e creare sockaddr

#include <poll.h>  //libreria per il polling del file descriptor (attendiamo che ci sia una richiesta i/o)

#define MAX_QUEUE 10 //numero massimo di richieste di connessioni permesse in un dato momento

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;

void print_addr(sockaddr *addr){
	printf("\n\nInfo:\n\n");
	printf("FAMILY: %d\n", addr->sa_family);
	sockaddr_in * addr_in = (sockaddr_in *) addr;
	printf("ADDR: %s \nPORT: %d\n", inet_ntoa(addr_in->sin_addr), ntohs(addr_in->sin_port));
}

/*int file_descriptor(){
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
		exit(-1);
	}

	return sockfd;
}*/


//When  a  socket is created with socket(2), it exists in a name space (address family) but has no address assigned to it.  bind() assigns the address
int make_socket(char* port){
	
	//USEREMO 
	//sockfd specifica il socket da bindare (ottenuto con socket()), la struttura addr ha tutte le informazioni sull'indirizzo da bindare e la famiglia di protocolli utilizzata, addrlen serve per indicare quanti dati sono in addr

	//prepariamo i parametri da passare

	/*The <netinet/in.h> header shall define the sockaddr_in structure, which shall include at least the following members:
		
		struct sockaddr_in{
           sa_family_t     sin_family;   //ex AF_INET.
           in_port_t       sin_port;     //Port number.
           struct in_addr  sin_addr;     //IP address.
		};

		struct in_addr {
            in_addr_t s_addr;
        };

     The <sys/socket.h> instead defines sockaddr structure as:
		
		struct sockaddr{
			sa_family_t  sa_family;  //Address family.
           	char         sa_data[];  //Socket address and other data(variable-length data). 
		};
    */

	/*//Per indicare di voler bindare il socket a tutte le interfacce di rete useremo la costante INADDR_ANY (se volessimo per esempio solo il local host potremmo usare invece una roba del tipo 127.0.0.1)
	
	//ora, sia l'indirizzo in_addr_t e in_port_t sono salvati in bigendian (host byte order), invece vogliamo siano nell'ordine del network (Network byte order). per fare ciò abbiamo htonl() e htons() che sono host-to-network

	{ //settiamo a
		(*addr_in).sin_addr.s_addr = htonl(INADDR_ANY);//inet_addr("127.0.0.1"); //impostiamo l'indirizzo
		(*addr_in).sin_family = AF_INET;  //impostiamo la famiglia
		(*addr_in).sin_port = htons(port);  //scegliamo la porta
	}

	print_addr_in(addr_in);

	sockaddr *addr = (sockaddr *) addr_in; //facciamo il casting

	socklen_t addrlen = sizeof(*addr_in); //addrlen specifies the size, in bytes, of the address structure pointed to by addr.*/

	/*------------------------------------------------------------------------------------------------------------------------------------------------*/

	//rispetto a quanto scritto sopra, ci semplifichiamo la vita utilizzando la funzione 

	//int getaddrinfo(const char *node, const char *service, const struct addrinfo *hints, struct addrinfo **res);

	//Given node and service, which identify an Internet host and a service. (ip + port)
	//getaddrinfo() returns one or more addrinfo structures, each of which contains an Internet address that can be specified in a call to bind(2) or connect(2).

	/* struct addrinfo {
               int              ai_flags;
               int              ai_family;
               int              ai_socktype;
               int              ai_protocol;
               socklen_t        ai_addrlen;
               struct sockaddr *ai_addr;
               char            *ai_canonname;
               struct addrinfo *ai_next;
           };
    */

    /* The  hints  argument  points to an addrinfo structure that specifies criteria for selecting the socket address
       structures returned in the list pointed to by res.  If hints is not NULL it points to  an  addrinfo  structure
       whose ai_family, ai_socktype, and ai_protocol specify criteria that limit the set of socket addresses returned
       by getaddrinfo() */

	/*con la prima struttura della lista che ci viene ritornata, creiamo il socket e facciamo il binding.*/

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
		if(errno == EACCES) printf("L'indirizzo con qui si sta eseguendo il binding e' protetto.");
		else if(errno == EADDRINUSE) printf("Indirizzo bindato gia in uso");
		else if(errno == EINVAL) printf("addrlen e'sbagliato oppure addr non e'un indirizzo valido");
		else if(errno == EADDRNOTAVAIL) printf("Interfaccia non esistente");
		else if(errno == ENAMETOOLONG) printf("address troppo lungo");
		fprintf(stderr, "Error binding\n");
		close(sockfd);
		freeaddrinfo(result);
		exit(-1);
	}
    
    print_addr(result->ai_addr);
    freeaddrinfo(result);

    return sockfd;
}

void accepting(int sockfd){
    /* int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen); */
	/*  If no pending connections are present on the queue, and the socket is not marked as nonblocking, accept() blocks the caller until a connection is present.  If the socket is marked nonblocking  and
       no pending connections are present on the queue, accept() fails with the error EAGAIN or EWOULDBLOCK. */
    sockaddr_in peer_addr;
    socklen_t peer_addrlen = sizeof(peer_addr);

    memset(&peer_addr, 0, sizeof(peer_addr));

    
    accept(sockfd, (sockaddr *)&peer_addr, &peer_addrlen);  //al ritorno dalla funzione, riceveremo l'indirizzo del peer in peer_addr
    printf("Richiesta in arrivo!\n");
    printf("Accettata!\n");

    print_addr((sockaddr *)&peer_addr);
    return;
    /*peer_addrlen on return it will contain the actual size of the peer address.
      and peer_addr the address itself
    
      we can use getsockname() to get local socket address and getpeername() the peer address.    
    */

    
}

int main(int argc, char* argv[]){

	if(argc < 2){
		printf("Usare: %s [port]",argv[0]);
	}

		/*To accept connections, the following steps are performed:

           1.  A socket is created with socket(2).

           2.  The socket is bound to a local address using bind(2), so that other sockets may be connect(2)ed to it.

           3.  A willingness to accept incoming connections and a queue limit for incoming connections are specified with listen().

           4.  Connections are accepted with accept(2).
    	*/

	/*int sockfd = file_descriptor();
	
	printf("\nsockfd: %d\n",sockfd);

	sockaddr *addr;
	sockaddr_in addr_in;

	addr = (sockaddr *) &addr_in;  //grazie a questo casting, le funzioni della libreria socket possono leggere addr_in tramite il puntatore addr*/

	//mando un doppio puntatore in modo da aggiornare il puntatore senza dover fare un return da binding
	int sockfd = make_socket(argv[1]);  //bindiamo il socket alla nostra porta e indirizzo

	/*When listen(2) is called on an unbound socket, the socket is automatically bound to a random free port with the local address set to INADDR_ANY. */
	// 
    
    errno = 0;
	int value = listen(sockfd, MAX_QUEUE);  //sockfd è il descrittore del socket, mentre MAX_QUEUE è il numero massimo di connessioni ascoltabili in un dato momento

	if(value == -1){
		if(errno == EADDRINUSE) printf("Errore, un'altro socket è in ascolto sulla porta scelta");
		return -1;
	}

	/* int poll(struct pollfd *fds, nfds_t nfds, int timeout); 

	poll() performs a similar task to select(2): it waits for one of a set of file descriptors to become ready to perform I/O.

	struct pollfd {
               int   fd;          //The field fd contains a file descriptor for an open file.
               short events;      //The  field  events is an input parameter, a bit mask specifying the events the application is interested in for the file descriptor fd. The values must be constructed by OR'ing some flags descripted in poll(3)
               short revents;     //The field revents is an output parameter, filled by the kernel with the events that actually occurred. The bits returned in revents can include any of those specified in events, or one of the values POLLERR, POLLHUP, or POLLNVAL.
    };
	
	The caller should specify the number of items in the fds array in nfds.

	The timeout argument specifies the number of milliseconds that poll() should block waiting for a file descriptor to become ready.

	Il blocco sussiste fino a quando il file descriptor non è pronto o finisce il timeout (se infinito non finirà mai). Specifying a negative value in timeout means an infinite timeout.
	*/

	struct pollfd fds;

	fds.fd = sockfd;
	fds.events = POLLIN; //ci aspettiamo dati di normali e di priorità senza essere bloccati

	while(1){
		printf("\n\nIn attesa di connessioni...\n");
		memset(&(fds.revents), 0, sizeof(fds.revents));  //resettiamo ogni volta revents per poter avere una descrizione sempre nuova di come è andato il polling
		errno = 0;
		value = poll(&fds, 1, -1);  //polliamo 1 file per INFINITO tempo

		if(value == -1){
			if(errno == EINTR) printf("Segnale catturato durante il polling");
			return -1;
		}

		//leggiamo revents per capire se possiamo accettare la richiesta di connessione

		if((fds.revents & POLLHUP) || (fds.revents & POLLERR) || (fds.revents & POLLNVAL)){
			printf("Errore con il socket, disconnessione dal socket in corso...");
			return -2;
		}

		if(fds.revents & POLLIN){
			/*qua facciamo il thread che si occupa della lettura e accettazione */
            accepting(sockfd);

		}
	}

	//per poter connettersi da fuori il pc, è necessario fare un forwarding dall'ip dell'host a quello del wsl

	/*netsh interface portproxy add v4tov4 listenport=<yourPortToForward> listenaddress=0.0.0.0 connectport=<yourPortToConnectToInWSL> connectaddress=(wsl hostname -I)*/
}
