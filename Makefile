# Makefile for CPE464 library

CC = gcc
CFLAGS = -g -Wall

OS = $(shell uname -s)
ifeq ("$(OS)", "SunOS")
	LIBS += -lsocket -lnsl
endif

LIBS += -lstdc++

SRCS = $(shell ls *.cpp *.c 2> /dev/null)
OBJS = $(shell ls *.cpp *.c 2> /dev/null | sed s/\.c[p]*$$/\.o/ )
LIBNAME = $(shell ls *cpe464*.a)

ALL = cclient server

all: $(OBJS) $(ALL)

lib:
	make -f lib.mk

echo:
	@echo "Objects: $(OBJS)"
	@echo "LIBNAME: $(LIBNAME)"

.cpp.o:
	@echo "-------------------------------"
	@echo "*** Building $@"
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)

.c.o:
	@echo "-------------------------------"
	@echo "*** Building $@"
	$(CC) -c $(CFLAGS) $< -o $@ $(LIBS)

rcopy: rcopy.c util.c
	@echo "-------------------------------"
	@echo "*** Linking $@ with library $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ $^ $(LIBNAME) $(LIBS)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

server: chat_server.c chat.c
	@echo "-------------------------------"
	@echo "*** Linking $@ with library $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ $^ $(LIBNAME) $(LIBS)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

cclient: chat_cclient.c chat.c
	@echo "-------------------------------"
	@echo "*** Linking $@ with library $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ $^ $(LIBNAME) $(LIBS)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

test: test.c
	@echo "-------------------------------"
	@echo "*** Linking $@ with library $(LIBNAME)... "
	$(CC) $(CFLAGS) -o $@ $^ $(LIBNAME)
	@echo "*** Linking Complete!"
	@echo "-------------------------------"

# clean targets for Solaris and Linux
clean: 
	@echo "-------------------------------"
	@echo "*** Cleaning Files..."
	rm -f *.o $(ALL)
	@echo "-------------------------------"
