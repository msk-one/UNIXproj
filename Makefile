all: gd_client gd_server
gd_client: client.c
	gcc -Wall -o gd_client client.c -lpthread
gd_server: server.c
	gcc -Wall -o gd_server server.c -lpthread
.PHONY: clean
clean:
	-rm gd_client gd_server