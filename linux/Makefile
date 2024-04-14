all: server_unix client_unix
	@echo File compilati

server_unix : server_unix.c
	gcc server_unix.c -o ../bin/server_unix.out -D_GNU_SOURCE

client_unix : client_unix.c
	gcc client_unix.c -o ../bin/client_unix.out -D_GNU_SOURCE
