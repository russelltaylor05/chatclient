#ifndef CHAT_HEADER
#define CHAT_HEADER

#define max(a,b) \
 ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
   _a > _b ? _a : _b; })
   
#define MAX_CLIENTS 100
#define MAX_MSG 1000
#define MAX_HANDLE 100
#define READ_BUFFER 2048


#pragma pack(1)
typedef struct packet_head {
  u_int32_t seq_num;
  u_int16_t checksum;
  u_int8_t  flag;
} PACKETHEAD;



// Server
int tcp_server_setup(int port);
int selectLoop(int server_socket);
int setupNewClient(int server_socket, int *client_sockets);
int fdsMax(int server_socket, int *client_sockets);

int takeAction(char *buffer, int active_socket, int *client_sockets, char **handle_table);
int addHandle(char *buffer, int active_socket, int *client_sockets, char **handle_table);

int removeSocket(int activeSocket, int *client_sockets, char **handle_table);
int countClients(int *client_sockets);


// Client
int tcp_send_setup(char *host_name, char *port);
int setupHandle(char *send_buf, char *handle, int seqNum);


/* General */
int grabHeader(PACKETHEAD *header, char *buffer, int len);  
void printHeader(PACKETHEAD *header);

int buildSimpleHeader(char *buffer, int flag, int seqNum);
void printClients(int *client_sockets, char **handle_table);

#endif
