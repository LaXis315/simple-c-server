all: server_win client_win
	@echo File compilati

server_win : server_win.c
	gcc server_win.c -o ..\bin\server_win -lws2_32

client_win : client_win.c
	gcc client_win.c -o ..\bin\client_win -lws2_32
