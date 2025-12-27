CC := clang
CFLAGS := -c
LFLAGS := -lncurses

all: server client evilserver

server: server.o conway.o message.o
	$(CC) $(LFLAGS) $^ -o $@

client: client.o conway.o message.o
	$(CC) $(LFLAGS) $^ -o $@

evilserver: evilserver.o conway.o message.o
	$(CC) $(LFLAGS) $^ -o $@

server.o: server.c conway.h
	$(CC) $(CFLAGS) $< -o $@

client.o: client.c
	$(CC) $(CFLAGS) $< -o $@

conway.o: conway.c conway.h
	$(CC) $(CFLAGS) $< -o $@

evilserver.o: evilserver.c conway.h
	$(CC) $(CFLAGS) $< -o $@

message.o: message.c message.h
	$(CC) $(CFLAGS) $< -o $@
