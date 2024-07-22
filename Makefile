CFLAGS = -Wall -g -Werror -Wno-error=unused-variable

all: server subscriber

server.o:
	g++ ${CFLAGS} -c server.cpp

common.o: common.cpp

server: server.o common.o
	g++ ${CFLAGS} server.o common.o -o server

subscriber: subscriber.cpp common.o

.PHONY: clean run_server run_subscriber

clean:
	rm -rf server subscriber *.o *.dSYM


