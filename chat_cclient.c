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


int main(int argc, char * argv[])
{

  char *handle, *host, *port;
  double error;
  int socket_num;
      
  handle = argv[1];
  error = atof(argv[2]);
  host = argv[3];
  port = argv[4];
  
  if (argc != 5) { printf("usage: %s host-name port-number \n", argv[0]); exit(1); }
  if (strlen(handle) > MAX_HANDLE) { printf("Handle name is too long\n"); exit(1); }
  
  sendErr_init(error, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_OFF);
  socket_num = tcp_send_setup(host, port);  
  
  clientLoop(socket_num, handle);
    
  close(socket_num);
  
  return 0;
}



/**********************/
/*   STATES/CONTROL   */
/**********************/

int clientLoop(int socket_num, char *handle)
{
  char *send_buf, *read_buf, **activePeers;
  int message_len, size;
  int send_len = 0, checksum, seqNum, exitFlag;
  int selectActive, state, waitCnt = 0;
  int msgSeqTracker[MAX_CLIENTS];
  uint32_t handleCount, currentHandle;
  
  send_buf = (char *) malloc(READ_BUFFER);
  read_buf = (char *) malloc(READ_BUFFER);
  if (!send_buf || !read_buf) {
    perror("malloc"); exit(EXIT_FAILURE);
  }

  memset((int *)msgSeqTracker, 0, sizeof(msgSeqTracker));
  activePeers = setupPeerTracking();
  
  state = HANDLE_SEND;
  currentHandle = 1;
  seqNum = 1;
  exitFlag = 0;
  
  while(!exitFlag) {
    
    selectActive = selectSetup(socket_num, state);
    
    if (selectActive == SOCKET_ACTIVE) {      
      memset(read_buf, 0, READ_BUFFER);
      if((message_len = recv(socket_num, read_buf, READ_BUFFER, 0)) < 0) { perror("rec Error: "); exit(EXIT_FAILURE); }
      if (!(checksum = in_cksum((unsigned short *)read_buf, message_len))){
        exitFlag = handleResponse(read_buf, 
                                  &state, 
                                  &seqNum, 
                                  socket_num, 
                                  activePeers, 
                                  msgSeqTracker, 
                                  &handleCount, 
                                  &currentHandle);
      }      
  
    } else if (selectActive == STDIN_ACTIVE) {
      
      send_len = 0;
      while ((send_buf[send_len] = getchar()) != '\n') {
        send_len++;
      }
      send_buf[send_len] = '\0';      
      
      if(!parseCommand(send_buf, send_len, &state)) {
        printf("Command Uknown\n");
        printPrompt();
        state = 0;        
      }        
    }  
    
    switch (state) {
    case HANDLE_SEND:
      size = buildHandleHeader(send_buf, handle, 1, seqNum);
      if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  
      state = HANDLE_WAIT;
      waitCnt = 0;
      break;
  
    case HANDLE_WAIT:
      waitCnt++;
      if(waitCnt >= 2) {
        state = HANDLE_SEND;
      }      
      break;
  
    case MSG_SEND:
      sendMsg(send_buf, socket_num, seqNum, handle);
      state = MSG_WAIT;
      waitCnt = 0;
      break;
  
    case MSG_WAIT:
      waitCnt++;
      if(waitCnt >= 2) {
        state = MSG_SEND;
      }      
      break;  

    case EXIT_SEND:
      size = buildSimpleHeader(send_buf, 8, seqNum);
      if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  
      state = EXIT_WAIT;
      waitCnt = 0;
      break;  

    case EXIT_WAIT:
      waitCnt++;
      if(waitCnt >= 2) {
        state = EXIT_SEND;
      }      
      break;
    
    case HANDLE_CNT_SEND:
      size = buildSimpleHeader(send_buf, 10, seqNum);
      if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }  
      state = HANDLE_CNT_WAIT;
      waitCnt = 0;
      break;  

    case HANDLE_CNT_WAIT:
      waitCnt++;
      if(waitCnt >= 2) {
        state = HANDLE_CNT_SEND;
      }      
      break;

    case HANDLE_REQ_SEND:
      size = buildCntHeader(send_buf, 12, seqNum, currentHandle);    
      if (sendErr(socket_num, send_buf, size, 0) < 0) { perror("Send:"); exit(EXIT_FAILURE);}      
      state = HANDLE_REQ_WAIT;
      waitCnt = 0;      
      break;

    case HANDLE_REQ_WAIT:      
      waitCnt++;
      if(waitCnt >= 2) {
        state = HANDLE_REQ_SEND;
      }      
      break;

  
    default:
      break;
    }
  }
  
  freePeerTracking(activePeers);
  free(send_buf);
  free(read_buf);
  
  return 0;
}


int selectSetup(int socket_num, int state)
{
  fd_set readfds;
  struct timeval timeout;

  FD_ZERO(&readfds);
  FD_SET(0, &readfds);           // STDIN
  FD_SET(socket_num, &readfds);  // server  
  timeout.tv_sec = 0;
  timeout.tv_usec = 100000;
  
  if (state != 0) {
    if(select(socket_num + 1, &readfds, NULL, NULL, &timeout) == -1) { perror("Select"); exit(EXIT_FAILURE); }
  } else {
    if(select(socket_num + 1, &readfds, NULL, NULL, NULL) == -1) { perror("Select"); exit(EXIT_FAILURE); }        
  }
 
  if (FD_ISSET(socket_num, &readfds)) {
    return SOCKET_ACTIVE;
  } else if (FD_ISSET(0, &readfds)) {
    return STDIN_ACTIVE;
  }
  return 0;

}


int handleResponse(char *read_buf, 
                   int *state, 
                   int *seqNum, 
                   int socket_num, 
                   char **activePeers, 
                   int *msgSeqTracker,
                   uint32_t *handleCount,
                   uint32_t *currentHandle) 
{
  PACKETHEAD header;
  int exitFlag = 0;
  char tmpHandle[MAX_HANDLE];
  memcpy(&header, read_buf, sizeof(PACKETHEAD));    

  switch (header.flag) {
  case 2: // good handle
    *state = 0;
    *seqNum = *seqNum + 1;
    break;

  case 3: // bad handle
    printf("Handle already taken. Exiting now ...\n");
    exitFlag = 1;
    break;

  case 6: // receiving msg
    printMsg(read_buf, activePeers, msgSeqTracker, *state);
    sendMsgAck(read_buf, socket_num, *seqNum);
    break;
    
  case 7: // handle doesn't exist
    memcpy(tmpHandle, &(read_buf[sizeof(PACKETHEAD) + 1]), read_buf[sizeof(PACKETHEAD)]);
    tmpHandle[(int)read_buf[sizeof(PACKETHEAD)]] = '\0';
    printf("\nClient with handle %s does not exist.\n", tmpHandle);  
    *state = 0;
    break;

  case 9: // Server ACKs exit request
    exitFlag = 1;
    break;

  case 11: // Server ACKs cnt request
    *handleCount = (uint32_t)grabCntHeader(read_buf);
    *state = HANDLE_REQ_SEND;
    *seqNum = *seqNum + 1;
    break;

  case 13: // Server ACKs cnt request    
    grabHandleHeader(read_buf, (char *)tmpHandle);
    printf("%d) %s\n", *currentHandle, tmpHandle);
    if(*currentHandle == *handleCount) {
      *currentHandle = 1;
      *state = 0;
    } else {
      (*currentHandle)++;
      *state = HANDLE_REQ_SEND;        
    }
    *seqNum = *seqNum + 1;
    break;
    
  case 255: // message success
    *state = 0;
    *seqNum = *seqNum + 1;
    break;

  default:
    break;
  }  

  if(*state == 0 && header.flag != 6) {
    printPrompt();    
  }

  return exitFlag;
}




int parseCommand(char *buffer, int length, int *state) 
{
  char command[3];
  char *pnt, *msgStart;
  int strLength = strlen(buffer);

  if(strLength < 2) {
    return 0;
  }

  memcpy(command, buffer, 2);
  command[2] = '\0';
  
  if(!strcmp(command, "%M") || !strcmp(command, "%m")) {
    // check for space
    if (!(pnt = strchr(buffer, ' '))) return 0;
    
    // check for space in position3
    if((pnt - buffer + 1) != 3) return 0;
    
    // check msg length
    msgStart = pnt + 1;
    while (*msgStart != ' ' && *msgStart != '\0') {
      msgStart++;
    }
    if(strlen(msgStart) >= 1000) {
      printf("Message is too long.\n");
      return 0;
    }    
    *state = MSG_SEND;    
  } else if (!strcmp(command, "%L") || !strcmp(command, "%l")) {
    *state = HANDLE_CNT_SEND;
  } else if (!strcmp(command, "%E") || !strcmp(command, "%e")) {
    *state = EXIT_SEND;
  } else {
    return 0;
  }
  
  return 1;
}

void printPrompt()
{
  printf(">");
  fflush(stdout);
}



/*******************/
/*     MESSAGES    */
/*******************/


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
  if (*pointer == '\0') {
    *pointer = ' ';
    *(pointer+1) = '\0';
  }
  msgPointer = pointer + 1;
  
  
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
  memcpy(&(sendBuffer[locater]), msgPointer, strlen(msgPointer) + 1);
  
  locater += strlen(msgPointer) + 1;
  totalSize = locater;
  
  header.checksum =  in_cksum((unsigned short *)sendBuffer, totalSize);
  memcpy(sendBuffer, &header, sizeof(PACKETHEAD));  
  memcpy(&header, sendBuffer, sizeof(PACKETHEAD));
  
  if (sendErr(socket_num, sendBuffer, totalSize, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }
  return 0;
}

void printMsg(char *buffer, char **activePeers, int *msgSeqTracker, int state) 
{
  char senderHandle[MAX_HANDLE];
  char receiverHandle[MAX_HANDLE];
  PACKETHEAD header;
  int peerId;
  int receiverLength = buffer[sizeof(PACKETHEAD)]; 
  int senderStart = sizeof(PACKETHEAD) + buffer[sizeof(PACKETHEAD)] + 2;
  int senderLength = buffer[senderStart -1];  
  int msgStart = senderStart + senderLength;
      
  memcpy((char *)senderHandle, &(buffer[senderStart]), senderLength);
  senderHandle[senderLength] = '\0';
  memcpy((char *)receiverHandle, &(buffer[sizeof(PACKETHEAD) + 1]), receiverLength);
  receiverHandle[receiverLength] = '\0';
  
  memcpy(&header, buffer, sizeof(PACKETHEAD));

  if ((peerId = checkPeerExists(senderHandle, activePeers)) < 0) {
    peerId= addPeer(senderHandle, activePeers);
    msgSeqTracker[peerId] = 0;
  }
  
  if(msgSeqTracker[peerId] < ntohl(header.seq_num)) {
    if(strcmp(receiverHandle, senderHandle)) {
      printf("\n");
    }
    printf("%s: %s\n", senderHandle, &(buffer[msgStart]));  
    msgSeqTracker[peerId] = ntohl(header.seq_num);
    if(state != MSG_WAIT) {
      printPrompt();
    }
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
  
  if (sendErr(socket_num, ackBuffer, locator, 0) < 0) { perror("Send Error: "); exit(EXIT_FAILURE); }
}



/*******************/
/*     PEERS       */
/*******************/

char **setupPeerTracking()
{ 
  int i;
  char **activePeers;
  
  activePeers = (char **)malloc(sizeof(char *) * MAX_CLIENTS);
  if(!activePeers) { perror("malloc"); exit(EXIT_FAILURE); }  
  for(i = 0; i < MAX_CLIENTS; i++) {
    activePeers[i] = (char *)malloc(sizeof(char) * MAX_HANDLE);
    if (!activePeers[i]) { perror("malloc"); exit(EXIT_FAILURE); }
    activePeers[i][0] = '\0';
  }  
  return activePeers;
}

void freePeerTracking(char **activePeers)
{ 
  int i;  
  for(i = 0; i < MAX_CLIENTS; i++) {
    free(activePeers[i]);
  }
  free(activePeers);
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

int addPeer(char *handle, char **activePeers) 
{
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

void printPeerTable(char **activePeers, int *msgSeqTracker) 
{
  int i;
  for (i = 0; i < MAX_CLIENTS; i++) {
    if(strlen(activePeers[i])) {
      printf("%d: \t%s\n", msgSeqTracker[i], activePeers[i]);
    }
  }    
}





/*******************/
/*   TCP SETUP     */
/*******************/

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



