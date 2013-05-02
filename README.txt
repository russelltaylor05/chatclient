cal poly username: rtaylor


SERVER:
usage: server error-percent requested-port
	request-port will try to setup the server using a specific port.
	If left blank a random available port will be choosen.
	EX. server .05 53001
	
	
CCLIENT:
usage: cclient host-name port-number
%M, %L, %E functionality all should be working. 

Known Errors
- Program will break if you try to send a message while it is waiting for an ACK from previous message. 