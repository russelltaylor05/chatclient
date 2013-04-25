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

main(int argc, char *argv[])
{
    int server_socket= 0;   //socket descriptor for the server socket
    int client_socket= 0;   //socket descriptor for the client socket
    char *buf;              //buffer for receiving from client
    int buffer_size= 1024;  //packet size variable
    int message_len= 0;     //length of the received message
    
    int socket_fd;
    //struct timeval tv;
    int flag;
    int n = 0;
    int num_fds;

    fd_set fds, readfds;
    int i, clientaddrlen;
    int client_sockets[MAX_CLIENTS];
    int rc, numsocks = 0;
    int active_socket, new_socket;
    
    for(i = 0; i < MAX_CLIENTS; i++) {
      client_sockets[i] = 0;
    }
    
    //create packet buffer
    buf = (char *) malloc(buffer_size);

    //printf("sockaddr: %d sockaddr_in %d\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in));
    
    //create the server socket
    printf("--- Server Started ----\n");
    server_socket = tcp_server_setup(53001);
    printf("Server Socket: %d\n\n", server_socket);

    if (listen(server_socket, 5) == -1) {
      perror("Listen");  
    }
    
    while(1) {

      // Populate Socket Sets
      FD_ZERO(&fds);
      FD_SET(server_socket, &fds);
      for ( i = 0 ; i < MAX_CLIENTS ; i++) {
        if(client_sockets[i] > 0) {
          FD_SET(client_sockets[i], &fds);
        }
      }

      readfds = fds;
      num_fds = fdsMax(server_socket, client_sockets, numsocks);
      printf("Connected Clients: %d\n", numsocks);
      printf("Max fds: %d\n", num_fds);
      
      if((rc = select(num_fds, &readfds, NULL, NULL, NULL)) == -1) {
        perror("Select Error: ");
        exit(EXIT_FAILURE);
      }

      // Server Socket Activity 
      if (FD_ISSET(server_socket, &readfds)) {
        new_socket = accept(server_socket, (struct sockaddr*)0, (socklen_t *)0);
        if (new_socket == -1) {
          perror("Accept");
          exit(EXIT_FAILURE);
        }                          
        //add new socket to array of sockets
        for (i = 0; i < MAX_CLIENTS; i++) {
          if (client_sockets[i] == 0) {
            client_sockets[i] = new_socket;
            printf("New Socket Added: %d\n", client_sockets[i]);
            i = MAX_CLIENTS;
            numsocks++;
          }
        }
      }          
    
      // Client Socket Activity 
      for (i = 0; i < MAX_CLIENTS; i++) {
        active_socket = client_sockets[i];
        if (FD_ISSET(active_socket, &readfds)) {

          /* Read returns 0 bytes if connections has been closed on other end */
          printf("Reading from socket: %d\n", active_socket);
          message_len = recv(active_socket, buf, buffer_size, 0);
          
          if (message_len < 0) {
            perror("recv call");
            exit(EXIT_FAILURE);
          } else if (message_len == 0) {
            printf("Socket %d disconnected\n", active_socket);
            client_sockets[i] = 0;
            close(active_socket);
            numsocks--;
          } else {              
            printf("Length: %d\n", message_len);
            printf("Data: %s\n", buf);
          }
    
        }
      }
      printf("\n");
    }
    
    /* close the sockets */
    close(server_socket);
}

/* This function sets the server socket.  It lets the system
   determine the port number.  The function returns the server
   socket number and prints the port number to the screen.  */

int tcp_server_setup(int port)
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
    local.sin_port= htons(port);         // 0 means system choose random available

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

    printf("Server port: %d \n", ntohs(local.sin_port));
	        
    return server_socket;
}

/* Returns the Max Socket File Descriptor +1 */
int fdsMax(int server_socket, int *client_sockets, int numsocks) 
{
  int max = 0;
  int i;
  
  max = max(server_socket, max);
  for (i = 0; i < MAX_CLIENTS; i++) {
    max = max(max, client_sockets[i]);
  }
  max++;
  return max;
}

