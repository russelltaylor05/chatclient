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
#include <time.h>

#include "cpe464.h"
#include "chat.h"

//cclient client1 .01 vogon 53002

#define STR(x)   #x
#define SHOW_DEFINE(x) printf("%s=%s\n", #x, STR(x))

int parseCommand(char *buffer, int length, int *state) 
{
  char command[3];
      
  memcpy(command, buffer, 2);
  command[2] = '\0';

  if(!strcmp(command, "%M") || !strcmp(command, "%m")) {
    *state = SEND_MSG;        
  } else if (!strcmp(command, "%L")) {
    
  } else if (!strcmp(command, "%E")) {
  
  } else {
    printf("\nCommand '%s' unknown.\n", command);
  }
  
  return 0; 
}


int handleResponse(char *read_buf, int *state, int *seqNum, int socket_num, char **activePeers, int *msgSeqTracker) 
{
  PACKETHEAD header;
  int response = 0;

  memcpy(&header, read_buf, sizeof(PACKETHEAD));  

  switch (header.flag) {
  case 2: // good handle
    *state = 0;
    *seqNum = *seqNum + 1;
    break;

  case 3: // bad handle
    printf("Handle already taken. Exiting now ...\n");
    *state = 0;
    response = 1;
    break;

  case 6: // receiving msg
    printMsg(read_buf, activePeers, msgSeqTracker);
    sendMsgAck(read_buf, socket_num, *seqNum);
    printPrompt();    
    break;
    
  case 7: // handle doesn't exist
    *state = HANDLE_NO_EXIST;
    break;
    
  case 255: // message success
    *state = 0;
    *seqNum = *seqNum + 1;
    break;

  default:
    break;
  }

  return response;
}



int main(int argc, char * argv[])
{
  int socket_num, message_len, size, i;
  char *send_buf, *read_buf;
  int send_len = 0;        
  char *handle, *host, *port;
  double error;
  int checksum;
  int seqNum = 1;
  int exitFlag = 0;
  int state = HANDLE_TRANSMIT;
  int temp;
  char tmpHandle[MAX_HANDLE];
  int waitCnt = 0;
  struct timeval timeout;

  /* peer msg tracking */
  char **activePeers;
  int msgSeqTracker[MAX_CLIENTS];
  memset((int *)msgSeqTracker, 0, sizeof(msgSeqTracker));
  activePeers = (char **)malloc(sizeof(char *) * MAX_CLIENTS);
  if(!activePeers) { perror("malloc"); exit(EXIT_FAILURE); }
  for(i = 0; i < MAX_CLIENTS; i++) {
    activePeers[i] = (char *)malloc(sizeof(char) * MAX_HANDLE);
    if (!activePeers[i]) { perror("malloc"); exit(EXIT_FAILURE); }
    activePeers[i][0] = '\0';
  }  
  
  
    
  handle = argv[1];
  error = atof(argv[2]);
  host = argv[3];
  port = argv[4];
  
  /* initialize data buffer for the packet */
  send_buf = (char *) malloc(READ_BUFFER);
  read_buf = (char *) malloc(READ_BUFFER);

  if (argc != 5) { printf("usage: %s host-name port-number \n", argv[0]); exit(1); }
  
  sendErr_init(error, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
  socket_num = tcp_send_setup(host, port);  
  

  printPrompt();
  while(!exitFlag) {

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(0, &readfds);  
    FD_SET(socket_num, &readfds);  
    timeout.tv_sec = 0;
    timeout.tv_usec = 500000;
    
    
    if (state != 0) {
      if((temp = select(socket_num + 1, &readfds, NULL, NULL, &timeout)) == -1) { perror("Select"); exit(EXIT_FAILURE); }
    } else {
      if((temp = select(socket_num + 1, &readfds, NULL, NULL, NULL)) == -1) { perror("Select"); exit(EXIT_FAILURE); }        
    }
    
    printf("state: %d\n", state);
    if (FD_ISSET(socket_num, &readfds)) {
      
      if((message_len = recv(socket_num, read_buf, READ_BUFFER, 0)) < 0) { perror("rec Error: "); exit(EXIT_FAILURE); }
      if ((checksum = in_cksum((unsigned short *)read_buf, message_len))) {
        printf("checksum error. msg_len: %d\n", message_len);        
      } else {
        exitFlag = handleResponse(read_buf, &state, &seqNum, socket_num, activePeers, msgSeqTracker);
      }      

    } else if (FD_ISSET(0, &readfds)) {
      
      send_len = 0;
      while ((send_buf[send_len] = getchar()) != '\n') {
        send_len++;
      }	   
      send_buf[send_len] = '\0';      
      parseCommand(send_buf, send_len, &state);
    }  
    
    switch (state) {
  
    case HANDLE_TRANSMIT:
      //printf("\nSTATE: HANDLE_TRANSMIT\n");
      printPrompt();
      size = setupHandle(send_buf, handle, seqNum);
      if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  
      state = HANDLE_WAIT;
      waitCnt = 0;
      break;

    case HANDLE_WAIT:  
      //printf("STATE: HANDLE_WAIT\n");
      waitCnt++;
      if(waitCnt >= 2) {
        state = HANDLE_TRANSMIT;
      }      
      break;
  
    case SEND_MSG:
      //printf("STATE: SEND_MSG\n");
      if((send_len = sendMsg(send_buf, socket_num, seqNum, handle)) < 0) {
        printf("bad msg format\n");  
      } else {
        state = MSG_WAIT;
        waitCnt = 0;
      }
      printPrompt();
      break;

    case MSG_WAIT:
      //printf("STATE: MSG_WAIT\n");
      waitCnt++;
      if(waitCnt <= 2) {
        state = SEND_MSG;
      }      
      break;
      
    //case RECV_MSG:
    //  break;
     
    case HANDLE_NO_EXIST:
      memcpy(tmpHandle, &(read_buf[sizeof(PACKETHEAD) + 1]), read_buf[sizeof(PACKETHEAD)]);
      tmpHandle[(int)read_buf[sizeof(PACKETHEAD)]] = '\0';
      printf("Client with handle %s does not exist.\n", tmpHandle);
      printPrompt();
      state = 0;
      break;  
  
    default:    
      break;
    }  
  }
  
  free(send_buf);
  free(read_buf);  
  close(socket_num);
  
  return 0;
}


int checkPeerExists(char *handle, char **activePeers)
{
  int i;
  for( i = 0; i < MAX_CLIENTS; i++) {
    if(strcmp(handle, activePeers[i]) == 0) {
      return i;
    }  
  }
  return -1;
}

int addPeer(char *handle, char **activePeers) {
  int i;
  //add new socket to array of sockets
  for (i = 0; i < MAX_CLIENTS; i++) {
    if(strlen(activePeers[i]) == 0) {
      memcpy(activePeers[i], handle, strlen(handle));
      return i;
    }
  }  
  return -1;
}

void printPeerTable(char **activePeers, int *msgSeqTracker) {
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if(strlen(activePeers[i])) {
      printf("%d: \t%s\n", msgSeqTracker[i], activePeers[i]);
    }
  }    
  
}

void printMsg(char *buffer, char **activePeers, int *msgSeqTracker) 
{
  char senderHandle[MAX_HANDLE];
  PACKETHEAD header;
  int peerId;
  
  int senderLength = buffer[sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + 1];
  int senderStart = sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + 2;
  int msgStart = sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + buffer[(int)buffer[sizeof(PACKETHEAD)]] + 2;
    
  memcpy((char *)senderHandle, &(buffer[senderStart]), senderLength);
  senderHandle[senderLength] = '\0';
  memcpy(&header, buffer, sizeof(PACKETHEAD));


  if ((peerId = checkPeerExists(senderHandle, activePeers)) < 0) {
    peerId= addPeer(senderHandle, activePeers);
    msgSeqTracker[peerId] = ntohl(header.seq_num);
  }  
  printf("--Peer Table--\n");
  printPeerTable(activePeers, msgSeqTracker);  
  if(msgSeqTracker[peerId] < ntohl(header.seq_num)) {
    printf("%s: %s\n", senderHandle, &(buffer[msgStart]));  
    msgSeqTracker[peerId] = ntohl(header.seq_num);
  } 
  
}


void sendMsgAck(char *buffer, int socket_num, int seqNum) 
{
  char ackBuffer[READ_BUFFER];
  int seqToAck;
  PACKETHEAD header;
  int senderLength = buffer[sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + 1];
  int senderStart = sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + 2;
  int locator = 0;
  
  memcpy(&header, buffer, sizeof(PACKETHEAD));
  seqToAck = header.seq_num;

  memset(&header, 0, sizeof(PACKETHEAD));  
  header.seq_num = htonl(seqNum);
  header.checksum = 0;
  header.flag = 255;

  memcpy(&(ackBuffer[locator]), &header, sizeof(PACKETHEAD));
  locator += sizeof(PACKETHEAD);
  
  memcpy(&(ackBuffer[locator]), &(seqToAck), sizeof(header.seq_num));
  locator += sizeof(header.seq_num);

  memcpy(&(ackBuffer[locator]), &(senderLength), 1);
  locator++;
  
  memcpy(&(ackBuffer[locator]), &(buffer[senderStart]), senderLength);
  locator += senderLength;
  
  header.checksum = in_cksum((unsigned short *)ackBuffer, locator);
  memcpy(ackBuffer, &header, sizeof(PACKETHEAD));
  
  //printf("send length: %d\n", locator);
  if (sendErr(socket_num, ackBuffer, locator, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }
}

int sendMsg(char *buffer, int socket_num, int seq, char *srcHandle) 
{

  int handleLength = 0;
  char *handlePointer, *msgPointer;
  char sendBuffer[READ_BUFFER];
  PACKETHEAD header;
  int locater = 0, totalSize;
  char *pointer;
    
  pointer = strchr(buffer, ' ') + 1;
  handlePointer = pointer;

  while(*pointer != ' ' && *pointer != '\0') {
    handleLength++;
    pointer++;
  }
  msgPointer = pointer + 1;
  
  if(!strlen(msgPointer) || !handleLength) {
    printf("Bad msg format");
    return -1;
  }

  memset(&header, 0, sizeof(PACKETHEAD));
  header.seq_num = htonl(seq);
  header.checksum = 0;
  header.flag = 6;
  
  locater = 0;
  memcpy(&(sendBuffer[locater]), &header, sizeof(PACKETHEAD)); 

  locater += sizeof(PACKETHEAD);
  sendBuffer[locater] = handleLength;

  locater += + 1;
  memcpy(&(sendBuffer[locater]), handlePointer, handleLength);

  locater += handleLength;
  sendBuffer[locater] = strlen(srcHandle);

  locater += 1;
  memcpy(&(sendBuffer[locater]), srcHandle, strlen(srcHandle));
  
  locater += strlen(srcHandle);
  memcpy(&(sendBuffer[locater]), msgPointer, strlen(msgPointer));
  
  locater += strlen(msgPointer);
  sendBuffer[locater] = '\0';
  
  totalSize = locater+1;
  header.checksum =  in_cksum((unsigned short *)sendBuffer, totalSize);
  memcpy(sendBuffer, &header, sizeof(PACKETHEAD));
  
  memcpy(&header, sendBuffer, sizeof(PACKETHEAD));
  
  /*
  printHeader(&header);  
  i = sizeof(PACKETHEAD);
  while(sendBuffer[i] != '\0') {
    printf("%c[%d]", sendBuffer[i], sendBuffer[i]);
    i++;
  }
  printf("\n");
  */
  
  if (sendErr(socket_num, sendBuffer, totalSize, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }
  return 0;
}


/* Setup send_buf to include header and handle name for transmision to the server */
int setupHandle(char *send_buf, char *handle, int seqNum) 
{
  PACKETHEAD header;
  int size = sizeof(PACKETHEAD) + 1 + strlen(handle);
  
  memset(&header, 0, sizeof(PACKETHEAD));
  header.seq_num = htonl(seqNum);
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


void printPrompt() 
{
  printf(">");
  fflush(stdout);
}
