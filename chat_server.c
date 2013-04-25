/******************************************************************************
 * tcp_server.c
 *
 * CPE 464 - Program 1
 *****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "chat.h"

#define max(a,b) \
 ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
   _a > _b ? _a : _b; })

main(int argc, char *argv[])
{
    int server_socket= 0;   //socket descriptor for the server socket
    int client_socket= 0;   //socket descriptor for the client socket
    char *buf;              //buffer for receiving from client
    int buffer_size= 1024;  //packet size variable
    int message_len= 0;     //length of the received message
    
    int socket_fd, result;
    struct timeval tv;
    int flag;
    int n = 0;
    int num_fds;

    fd_set fds, readfds;
    int i, clientaddrlen;
    int clientsock[2], rc, numsocks = 0, maxsocks = 2;

    printf("sockaddr: %d sockaddr_in %d\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in));
    
    //create packet buffer
    buf = (char *) malloc(buffer_size);

    //create the server socket
    server_socket = tcp_recv_setup();
    printf("server_socket: %d\n", server_socket);

    if (-1 == listen(server_socket, 5)) {
      perror("Listen");  
    }
    
    FD_ZERO(&fds);
    FD_SET(server_socket, &fds);

    while(n < 20) {
      readfds = fds;
      
      num_fds = 0;
      num_fds = max(server_socket, num_fds);
      for (i = 0; i < numsocks; i++) {
        num_fds = max(num_fds, clientsock[i]);
      }
      num_fds++;
      printf("num_fds: %d\n", num_fds);
      
      rc = select(num_fds, &readfds, NULL, NULL, NULL);    
      if (rc == -1) {
        perror("Select");
        break;
      }
    
      for (i = 0; i < num_fds; i++) {
        if (FD_ISSET(i, &readfds)) {
          printf("socket set: %d\n", i);
          if (i == server_socket) {
            if (numsocks < maxsocks) {
              clientsock[numsocks] = accept(server_socket,
                                        (struct sockaddr*)0,
                                        (socklen_t *)0);
              if (clientsock[numsocks] == -1) perror("Accept");
              FD_SET(clientsock[numsocks], &fds);
              printf("set new client sock: %d\n", clientsock[numsocks]);
              numsocks++;
            } else {
              printf("Ran out of socket space.\n");
            }
            
          } else {
            
            /* Read returns 0 bytes if connections has been closed on other end */
            //now get the data on the client_socket
            printf("reading from socket: [%d]%d\n",i, clientsock[i]);
            if ((message_len = recv(i, buf, buffer_size, 0)) < 0) {
              perror("recv call");
              exit(-1);
            }
            printf("Length: %d\n", message_len);
            printf("Data: %s\n", buf);
    
          }
        }
      }
      n++;
      printf("\n");
    }
    
    /* close the sockets */
    close(server_socket);
    close(client_socket);
}

/* This function sets the server socket.  It lets the system
   determine the port number.  The function returns the server
   socket number and prints the port number to the screen.  */

int tcp_recv_setup()
{
    int server_socket= 0;
    struct sockaddr_in local;      /* socket address for local side  */
    socklen_t len= sizeof(local);  /* length of local address        */

    /* create the socket  */
    server_socket= socket(AF_INET, SOCK_STREAM, 0);
    if(server_socket < 0) {
      perror("socket call");
      exit(1);
    }

    local.sin_family= AF_INET;           //internet family
    local.sin_addr.s_addr= INADDR_ANY;   //wild card machine address
    local.sin_port= htons(53005);            //let system choose the port

    /* bind the name (address) to a port */
    if (bind(server_socket, (struct sockaddr *) &local, sizeof(local)) < 0) {
      perror("bind call");
      exit(-1);
    }
    
    //get the port name and print it out
    if (getsockname(server_socket, (struct sockaddr*)&local, &len) < 0) {
      perror("getsockname call");
      exit(-1);
    }

    printf("socket has port %d \n", ntohs(local.sin_port));
    //printf("server_socket has port %d \n", server_socket);
	        
    return server_socket;
}

/* This function waits for a client to ask for services.  It returns
   the socket number to service the client on.    */

int tcp_listen(int server_socket, int back_log)
{
  int client_socket= 0;
  if (listen(server_socket, back_log) < 0) {
    perror("listen call");
    exit(-1);
  }
  
  if ((client_socket = accept(server_socket, (struct sockaddr*)0, (socklen_t *)0)) < 0) {
    perror("accept call");
    exit(-1);
  }
  
  return(client_socket);
}

