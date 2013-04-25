#ifndef CHAT_HEADER
#define CHAT_HEADER

#define max(a,b) \
 ({ __typeof__ (a) _a = (a); \
     __typeof__ (b) _b = (b); \
   _a > _b ? _a : _b; })
   
#define MAX_CLIENTS 100



typedef struct packet_head {
  u_int32_t seq_num;
  u_int16_t checksum;
  u_int8_t  flag;
} PACKETHEAD;



// for the server side
int tcp_server_setup(int port);
int tcp_listen(int server_socket, int back_log);

// for the client side
int tcp_send_setup(char *host_name, char *port);


int fdsMax(int server_socket, int *client_sockets, int num_sockets);


#endif
