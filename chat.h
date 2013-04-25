#ifndef CHAT_HEADER
#define CHAT_HEADER


typedef struct packet_head {
  u_int32_t seq_num;
  u_int16_t checksum;
  u_int8_t  flag;
} PACKETHEAD;



// for the server side
int tcp_recv_setup();
int tcp_listen(int server_socket, int back_log);

// for the client side
int tcp_send_setup(char *host_name, char *port);



#endif
