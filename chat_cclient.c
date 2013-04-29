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

#include "cpe464.h"
#include "chat.h"


int main(int argc, char * argv[])
{
  int socket_num, message_len, size;         //socket descriptor
  char *send_buf, *read_buf;         //data buffer
  int send_len= 0;        //amount of data to send
  int sent= 0;            //actual amount of data sent
  char *handle, *host, *error, *port;
  int exit_command = 0;
  PACKETHEAD header;
  int checksum, seqNum = 0;
  int successFlag = 0;
  fd_set readfds;
  struct timeval timeout;
    
  handle = argv[1];
  error = atoi(argv[2]);
  host = argv[3];
  port = argv[4];
  
  /* initialize data buffer for the packet */
  send_buf = (char *) malloc(READ_BUFFER);
  read_buf = (char *) malloc(READ_BUFFER);

  if (argc != 5) { printf("usage: %s host-name port-number \n", argv[0]); exit(1); }
  
  sendErr_init(error, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
  socket_num = tcp_send_setup(host, port);  
  

  size = setupHandle(send_buf, handle, seqNum);

  while(!successFlag) {
    
    FD_ZERO(&readfds);
    FD_SET(socket_num, &readfds);  
    timeout.tv_sec = 1;
    timeout.tv_usec = 10000;

    if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  

    if(select(socket_num + 1, &readfds, NULL, NULL, &timeout) == -1) { perror("Select"); exit(EXIT_FAILURE); }

    
    if (FD_ISSET(socket_num, &readfds)) {
      message_len = recv(socket_num, read_buf, READ_BUFFER, 0);
      if ((checksum = in_cksum((unsigned short *)read_buf, message_len))) {
        printf("checksum error\n");     
      } else {
        memcpy(&header, read_buf, sizeof(PACKETHEAD));  
        printHeader(&header);
        successFlag = 1;
      }      
    } 
  }
  if(header.flag == 3) {
    printf("Handle already taken. Exiting now ...\n");
    return 0;
  }
  
  
  while(!exit_command) {
      
    printf(">");
    
    send_len = 0;
    while ((send_buf[send_len] = getchar()) != '\n') {
      send_len++;  
    }	   
    send_buf[send_len] = '\0';
            
    sent = send(socket_num, send_buf, send_len, 0);
    if (sent < 0) {
      perror("send call");
      exit(-1);
    }
  
    printf("String sent: %s \n", send_buf);
    printf("Amount of data sent is: %d\n", sent);    
  }
  
  free(send_buf);
  free(read_buf);  
  close(socket_num);
  
  return 0;
}


/* Setup send_buf to include header and handle name for transmision to the server */
int setupHandle(char *send_buf, char *handle, int seqNum) 
{
  PACKETHEAD header;
  int size = sizeof(PACKETHEAD) + 1 + strlen(handle);
  
  memset(&header, 0, sizeof(PACKETHEAD));
  header.seq_num = seqNum;
  header.checksum = 0;
  header.flag = 1;
  
  memcpy(send_buf, &header, sizeof(PACKETHEAD)); 
  send_buf[sizeof(PACKETHEAD)] = strlen(handle);
  memcpy(&(send_buf[sizeof(PACKETHEAD) + 1]), handle, strlen(handle));

  header.checksum =  in_cksum((unsigned short *)send_buf, size);  
  memcpy(send_buf, &header, sizeof(PACKETHEAD));

  return size;
}


int tcp_send_setup(char *host_name, char *port)
{
  int socket_num;
  struct sockaddr_in remote;       // socket address for remote side
  struct hostent *hp;              // address of remote host

  // create the socket
  if ((socket_num = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket call");
    exit(-1);
  }
  
  // designate the addressing family
  remote.sin_family= AF_INET;

  // get the address of the remote host and store
  if ((hp = gethostbyname(host_name)) == NULL) {
    printf("Error getting hostname: %s\n", host_name);
    exit(-1);
  }
  
  memcpy((char*)&remote.sin_addr, (char*)hp->h_addr, hp->h_length);

  // get the port used on the remote side and store
  remote.sin_port= htons(atoi(port));

  if(connect(socket_num, (struct sockaddr*)&remote, sizeof(struct sockaddr_in)) < 0) {
    perror("connect call");
    exit(-1);
  }
  
  return socket_num;
}

