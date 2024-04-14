all: server_unix client_unix
	@echo File compilati

server_unix : server_unix.c
	gcc server_unix.c -o ../server_unix.lin -D_GNU_SOURCE

client_unix : client_unix.c
	gcc client_unix.c -o ../client_unix.lin -D_GNU_SOURCE
