default: server client
server: server.c users.o sockets.h
	gcc -O2 -g server.c users.o -o server
client: client.c sockets.h 
	gcc client.c -g -o client -O2
users.o: users.c users.h 
	gcc -c users.c
