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

#define HANDLE_SEND     1
#define HANDLE_WAIT     2
#define MSG_SEND        3
#define MSG_WAIT        4
#define EXIT_SEND       5
#define EXIT_WAIT       6
#define HANDLE_CNT_SEND 7
#define HANDLE_CNT_WAIT 8
#define HANDLE_REQ_SEND 9
#define HANDLE_REQ_WAIT 10

#define SOCKET_ACTIVE   1
#define STDIN_ACTIVE    2

#pragma pack(1)
typedef struct packet_head {
  u_int32_t seq_num;
  u_int16_t checksum;
  u_int8_t  flag;
} PACKETHEAD;



// Server
int tcp_server_setup(int port);
int serverLoop(int server_socket);
int setupNewClient(int server_socket, int *client_sockets);
int fdsMax(int server_socket, int *client_sockets);

int takeAction(char *buffer, int bufferLen, int active_socket, int *client_sockets, char **handle_table, int *seqNum);
int addHandle(char *buffer, int active_socket, int *client_sockets, char **handle_table);

void removeSocket(int activeSocket, int *client_sockets, char **handle_table);
int countClients(int *client_sockets);

int checkHandleExists(char *handle, char **handle_table, int *client_sockets);
void sendHandleNoExist(int active_socket, char *buffer);


// Client
int parseCommand(char *buffer, int length, int *state);
int tcp_send_setup(char *host_name, char *port);
void printPrompt();
int selectSetup(int socket_num, int state);
int clientLoop(int socket_num, char *handle);
int handleResponse(char *read_buf, 
                   int *state, 
                   int *seqNum, 
                   int socket_num, 
                   char **activePeers, 
                   int *msgSeqTracker,
                   uint32_t *handleCount,
                   uint32_t *currentHandle);


int sendMsg(char *buffer, int socket_num, int seq, char *srcHandle);
void printMsg(char *buffer, char **activePeers, int *msgSeqTracker, int state);
void sendMsgAck(char *buffer, int socket_num, int seqNum);

char **setupPeerTracking();
int checkPeerExists(char *handle, char **activePeers);
int addPeer(char *handle, char **activePeers);
void freePeerTracking(char **activePeers);
void printPeerTable(char **activePeers, int *msgSeqTracker);




/* General */
int grabHeader(PACKETHEAD *header, char *buffer, int len);  
void printHeader(PACKETHEAD *header);

int buildSimpleHeader(char *buffer, int flag, int seqNum);
int buildCntHeader(char *buffer, int flag, int seqNum, uint32_t count);
int buildHandleHeader(char *buffer, char *handle, int flag, int seqNum);

void printClients(int *client_sockets, char **handle_table);
uint32_t clientCount(char **handle_table);

int grabCntHeader(char *buffer);
int grabHandleHeader(char *buffer, char *handle);

#endif
