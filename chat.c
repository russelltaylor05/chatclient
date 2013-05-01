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



void printClients(int *client_sockets, char **handle_table) {
  int i;
  for(i =0; i < MAX_CLIENTS; i++) {
    if(client_sockets[i] != 0) {
      printf("\tSocket[%d]: %s\n", client_sockets[i], handle_table[i]);
    }
  }
}
