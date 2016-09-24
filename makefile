all: server.o client.o bank.o server client

server:
	gcc -Wall -Werror -g -o server server.c bank.c -lpthread
client: 
	gcc -Wall -Werror -g -o client client.c -lpthread
server.o: server.c bank.c bank.h
	gcc -Wall -Werror -c bank.c server.c
client.o: client.c
	gcc -Wall -Werror -c client.c
clean:
	rm *.o server client 