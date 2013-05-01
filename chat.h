#ifndef CHAT_HEADER
#define CHAT_HEADER

#define max(a,b) \
 ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
   _a > _b ? _a : _b; })
   
#define MAX_CLIENTS 100
#define MAX_MSG 1000
#define MAX_HANDLE 255
#define READ_BUFFER 2048

#define HANDLE_TRANSMIT 1
#define HANDLE_WAIT     2
#define HANDLE_EXIT     3
#define SEND_MSG        4
#define MSG_WAIT        5
#define HANDLE_NO_EXIST 6
#define RECV_MSG        7
#define MSG_ACK         8


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

int takeAction(char *buffer, int bufferLen, int active_socket, int *client_sockets, char **handle_table);
int addHandle(char *buffer, int active_socket, int *client_sockets, char **handle_table);

int removeSocket(int activeSocket, int *client_sockets, char **handle_table);
int countClients(int *client_sockets);

int checkHandleExists(char *handle, char **handle_table, int *client_sockets);
void sendHandleNoExist(int active_socket, char *buffer);


// Client
int tcp_send_setup(char *host_name, char *port);
int setupHandle(char *send_buf, char *handle, int seqNum);
int sendMsg(char *buffer, int socket_num, int seq, char *srcHandle);
void printMsg(char *buffer, char **activePeers, int *msgSeqTracker);
void printPrompt();
void sendMsgAck(char *buffer, int socket_num, int seqNum);


/* General */
int grabHeader(PACKETHEAD *header, char *buffer, int len);  
void printHeader(PACKETHEAD *header);

int buildSimpleHeader(char *buffer, int flag, int seqNum);
void printClients(int *client_sockets, char **handle_table);

#endif
