# Makefile for CPE464 program 1

CC= gcc
CFLAGS= -g

# The  -lsocket -lnsl are needed for the sockets.
# The -L/usr/ucblib -lucb gives location for the Berkeley library needed for
# the bcopy, bzero, and bcmp.  The -R/usr/ucblib tells where to load
# the runtime library.

# The next line is needed on Sun boxes (so uncomment it if your on a
# sun box)
# LIBS =  -lsocket -lnsl

# For Linux/Mac boxes uncomment the next line - the socket and nsl
# libraries are already in the link path.
LIBS =

all:   tcp_client tcp_server

tcp_client: tcp_client.c networks.h
	$(CC) $(CFLAGS) -o tcp_client tcp_client.c $(LIBS)

tcp_server: tcp_server.c networks.h
	$(CC) $(CFLAGS) -o tcp_server tcp_server.c $(LIBS)

clean:
	rm tcp_server tcp_client



