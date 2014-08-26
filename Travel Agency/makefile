#OBJ: DEP
#<TAB> CMD

all: Server Client Admin

Server: Server.o 
	gcc Server.o -o Server
	
Client: Client.o
	gcc Client.o -o Client

Admin: Admin.o
	gcc Admin.o -o Admin
	
Server.o: Server.c
	gcc -c Server.c

Client.o: Client.c
	gcc -c Client.c

Admin.o: Admin.c
	gcc -c Admin.c
	
clear:
	rm -f *.o
