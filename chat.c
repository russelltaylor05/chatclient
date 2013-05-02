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


int grabHeader(PACKETHEAD *header, char *buffer, int len)
{  
  return 0;
}

void printHeader(PACKETHEAD *header)
{
  printf("Packet Header\n");
  printf("  seq_num: \t%d\n", header->seq_num);
  printf("  checksum: \t%d\n", header->checksum);
  printf("  flag: \t%d\n", header->flag);
}


int buildSimpleHeader(char *buffer, int flag, int seqNum)
{
  PACKETHEAD header;
  int checksum;
  
  memset(&header, 0, sizeof(PACKETHEAD));
  header.seq_num = htonl(seqNum);
  header.checksum = 0;
  header.flag = flag;
  
  checksum = in_cksum((unsigned short *)&header, sizeof(PACKETHEAD));
  header.checksum = checksum;  
  memcpy(buffer, &header, sizeof(PACKETHEAD));

  return sizeof(PACKETHEAD);
}


/* Setup send_buf to include header and handle name for transmision to the server */
int buildHandleHeader(char *buffer, char *handle, int flag, int seqNum) 
{
  PACKETHEAD header;
  int size = sizeof(PACKETHEAD) + 1 + strlen(handle);
  
  memset(&header, 0, sizeof(PACKETHEAD));
  header.seq_num = htonl(seqNum);
  header.checksum = 0;
  header.flag = flag;
  
  memcpy(buffer, &header, sizeof(PACKETHEAD)); 
  buffer[sizeof(PACKETHEAD)] = strlen(handle);
  memcpy(&(buffer[sizeof(PACKETHEAD) + 1]), handle, strlen(handle));

  header.checksum =  in_cksum((unsigned short *)buffer, size);  
  memcpy(buffer, &header, sizeof(PACKETHEAD));

  return size;
}


int buildCntHeader(char *buffer, int flag, int seqNum, uint32_t count) 
{
  PACKETHEAD header;
  int size = sizeof(uint32_t) + sizeof(PACKETHEAD);
  
  memset(&header, 0, sizeof(PACKETHEAD));
  header.flag = flag;
  header.checksum = 0;
  header.seq_num = htonl(seqNum);
  memcpy(buffer, &header, sizeof(PACKETHEAD));
  
  memcpy(&(buffer[sizeof(PACKETHEAD)]), &count, sizeof(uint32_t));
  header.checksum =  in_cksum((unsigned short *)buffer, size);
  memcpy(buffer, &header, sizeof(PACKETHEAD));
  return size;
}

int grabCntHeader(char *buffer)
{
  uint32_t value;
  memcpy(&value, &(buffer[sizeof(PACKETHEAD)]), sizeof(uint32_t));
  return (int)value;
}

int grabHandleHeader(char *buffer, char *handle)
{
  int length = buffer[sizeof(PACKETHEAD)];

  memcpy(handle, &(buffer[sizeof(PACKETHEAD) + 1] ), length);
  handle[length] = '\0';
  return length;
}



uint32_t clientCount(char **handle_table) {
  int i;
  uint32_t count = 0;
    
  for( i = 0; i < MAX_CLIENTS; i++) {
    if(strlen(handle_table[i])) {
      count++;
    }    
  } 
  return count;
}

void printClients(int *client_sockets, char **handle_table) {
  int i;
  for(i =0; i < MAX_CLIENTS; i++) {
    if(client_sockets[i] != 0) {
      printf("\tSocket[%d]: %s\n", client_sockets[i], handle_table[i]);
    }
  }
}
