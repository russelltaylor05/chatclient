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


#include "cpe464.h"
#include "chat.h"



int main(int argc, char *argv[])
{
    int server_socket = 0;   //socket descriptor for the server socket        
    int requestedPort = 0;
    printf("sockaddr: %d sockaddr_in %d\n", sizeof(struct sockaddr), sizeof(struct sockaddr_in));
    
    if (argc < 2) {
      printf("usage: %s error-percent requested-port\n", argv[0]);
      printf("\trequest-port will try to setup server using that port.\n");
      printf("\tIf left blank a random available port will be choosen.\n");
      printf("\tEX. server .05 53001\n");       
      exit(1);
    }
    if(argc == 3) {
      requestedPort = atoi(argv[2]);
    }
    
    printf("--- Server Started ----\n");
    server_socket = tcp_server_setup(requestedPort);
    printf("Server Socket: %d\n\n", server_socket);

    sendErr_init(.5, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
    if (listen(server_socket, 5) == -1) { perror("Listen"); }
    
    serverLoop(server_socket);
    
    close(server_socket);
    return 0;
}


int serverLoop(int server_socket) 
{
    int active_socket, num_fds, i, message_len= 0, seqNum = 1;
    int client_sockets[MAX_CLIENTS];
    char *buf, **handle_table;
    fd_set readfds;
    
    handle_table = (char **)malloc(sizeof(char *) * MAX_CLIENTS);
    for(i = 0; i < MAX_CLIENTS; i++) {
      handle_table[i] = (char *)malloc(sizeof(char) * MAX_HANDLE);
    }

    memset(client_sockets, 0, sizeof(client_sockets));
    if(!(buf = (char *) malloc(READ_BUFFER))) {
      perror("Malloc Error: "); exit(EXIT_FAILURE);      
    }
        

    while(1) {

      // Populate Socket Sets
      FD_ZERO(&readfds);
      FD_SET(server_socket, &readfds);
      for ( i = 0 ; i < MAX_CLIENTS ; i++) {
        if(client_sockets[i] > 0) {
          FD_SET(client_sockets[i], &readfds);
        }
      }
                  
      num_fds = fdsMax(server_socket, client_sockets);
     
      if((select(num_fds, &readfds, NULL, NULL, NULL)) == -1) {
        perror("Select Error: ");
        exit(EXIT_FAILURE);
      }

      // New Client
      if (FD_ISSET(server_socket, &readfds)) {
        setupNewClient(server_socket, client_sockets);
      }          
    
      // Client Socket Activity 
      for (i = 0; i < MAX_CLIENTS; i++) {
        active_socket = client_sockets[i];
        if (FD_ISSET(active_socket, &readfds)) {
          memset(buf, 0, READ_BUFFER);
          message_len = recv(active_socket, buf, READ_BUFFER, 0);
          
          if (message_len < 0) {
            perror("recv call");
            exit(EXIT_FAILURE);
          } else if (message_len == 0) {
            //printf("Socket %d disconnected\n", active_socket);
            removeSocket(client_sockets[i], client_sockets, handle_table);
            close(active_socket);
          } else {
            if(!(in_cksum((unsigned short *)buf, message_len))) {
              takeAction(buf, message_len, active_socket, client_sockets, (char **)handle_table, &seqNum);
            }
          }
        }
      }
      //printf("\n");
    }
    return 0;
}


int takeAction(char *buffer, 
               int bufferLen, 
               int active_socket, 
               int *client_sockets, 
               char **handle_table,
               int *seqNum)
{
  PACKETHEAD header;
  char *send_buffer;
  char destHandle[MAX_HANDLE];
  int destSocket, size;
  int i, cnt;
  
  
  if((send_buffer = (char *) malloc(READ_BUFFER)) == NULL) {
    perror("malloc issue"); 
    exit(EXIT_FAILURE);    
  }
  memset(&header, 0, sizeof(PACKETHEAD));
  memcpy(&header, buffer, sizeof(PACKETHEAD));
  header.seq_num = ntohl(header.seq_num);
  
  switch (header.flag) {

  case 1: // Handle Add
    if (!(addHandle(buffer, active_socket, client_sockets, handle_table))){
      size = buildSimpleHeader(send_buffer, 3, 0);
    } else {
      size = buildSimpleHeader(send_buffer, 2, 0);
      //printf("Connected Clients: %d\n", countClients(client_sockets));
      //printClients(client_sockets, handle_table);
    }
    if (sendErr(active_socket, send_buffer, size, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}
    (*seqNum)++;
    break;

  case 6: // msg send
    size = buffer[sizeof(PACKETHEAD)];
    memcpy((char *)destHandle, &(buffer[sizeof(PACKETHEAD) + 1]), size);
    destHandle[size] = '\0';
    
    if(!(destSocket = checkHandleExists(destHandle, handle_table, client_sockets))) {
      sendHandleNoExist(active_socket, buffer);
    } else {
      if (sendErr(destSocket, buffer, bufferLen, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}
    }
    (*seqNum)++;
    break;
    
  case 8: // client exit request
    size = buildSimpleHeader(send_buffer, 9, 0);
    if (sendErr(active_socket, send_buffer, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  
    (*seqNum)++;
    break;

  case 10: //handle cnt request
    size = buildCntHeader(buffer, 11, *seqNum, clientCount(handle_table));    
    if (sendErr(active_socket, buffer, size, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}
    (*seqNum)++;
    break;

  case 12: //handle cnt request
    cnt = 0; i = 0;
    while(cnt < grabCntHeader(buffer) && i < MAX_CLIENTS) {
      if(strlen(handle_table[i]))
        cnt++;
      i++;  
    }
    size = buildHandleHeader(buffer, handle_table[i-1], 13, *seqNum);    
    if (sendErr(active_socket, buffer, size, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}
    (*seqNum)++;
    break;
    
  case 255: // MSG Ack
    size = buffer[sizeof(PACKETHEAD) + 4];
    memcpy((char *)destHandle, &(buffer[sizeof(PACKETHEAD) + 5]), size);
    destHandle[size] = '\0';
    destSocket = checkHandleExists(destHandle, handle_table, client_sockets);
    if (sendErr(destSocket, buffer, bufferLen, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}    
    (*seqNum)++;
    break;
    
  default:
    printf("Unknown Flag\n");
    break;
  }

  return 0;
}

// return 1 for success, 0 for duplicate handle
int addHandle(char *buffer, int active_socket, int *client_sockets, char **handle_table)
{

  int handle_size = buffer[sizeof(PACKETHEAD)];
  char temp_handle[255];
  int i;

  memcpy(temp_handle, &(buffer[sizeof(PACKETHEAD) + 1]), handle_size);  
  temp_handle[handle_size] = '\0';
  
  // dup check
  for( i = 0; i < MAX_CLIENTS; i++) {
    if(client_sockets[i] != 0 && client_sockets[i] != active_socket) {
      if(strcmp(temp_handle, handle_table[i]) == 0) {
        return 0;
      }      
    }    
  }
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] == active_socket) {
      memcpy(handle_table[i], (char *)temp_handle, handle_size);
      //printf("Handle added: %s\n", handle_table[i]);
    }     
  }
  return 1;
} 

int setupNewClient(int server_socket, int *client_sockets) 
{
  int new_socket;
  int i;
  
  new_socket = accept(server_socket, (struct sockaddr*)0, (socklen_t *)0);  
  if (new_socket == -1) {
    perror("Accept");
    exit(EXIT_FAILURE);
  }                          
  //add new socket to array of sockets
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] == 0) {
      client_sockets[i] = new_socket;
      //printf("New Socket Added: %d\n", client_sockets[i]);
      i = MAX_CLIENTS;
    }
  }
  return 0;
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
int fdsMax(int server_socket, int *client_sockets) 
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


void removeSocket(int activeSocket, int *client_sockets, char **handle_table) 
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] == activeSocket) {
      client_sockets[i] = 0;
      memset(handle_table[i], 0, MAX_HANDLE);
      handle_table[i][0] = '\0';
    }
  }
}

int countClients(int *client_sockets) {
  int i, cnt = 0;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if (client_sockets[i] != 0) {
      cnt++;
    }
  }
  return cnt; 
}

// returns socket # if it exists. 0 if it doesn't
int checkHandleExists(char *handle, char **handle_table, int *client_sockets)
{
  int i;
  for( i = 0; i < MAX_CLIENTS; i++) {
    if(client_sockets[i] != 0) {
      if(strcmp(handle, handle_table[i]) == 0) {
        return client_sockets[i];
      }      
    }    
  }
  return 0;
}

void sendHandleNoExist(int active_socket, char *buffer)
{
  PACKETHEAD header;
  int size = buffer[sizeof(PACKETHEAD)] + sizeof(PACKETHEAD) + 1;
  
  memset(&header, 0, sizeof(PACKETHEAD));
  memcpy(&header, buffer, sizeof(PACKETHEAD));  
  header.flag = 7;
  header.checksum = 0;
  
  memcpy(buffer, &header, sizeof(PACKETHEAD));
  header.checksum =  in_cksum((unsigned short *)buffer, size);
  memcpy(buffer, &header, sizeof(PACKETHEAD));
  
  if (sendErr(active_socket, buffer, size, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}

}
