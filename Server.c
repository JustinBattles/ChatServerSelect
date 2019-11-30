#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


/* includes */

#define K	1024
#define PORT	47308

fd_set active, rset;  

/**
 * Create a chat-server.  Things to consider:
 * 1) Create a client list structure or array.  Things to consider having is
 *    some way to mark a client as being dead before it is removed.  Anytime a
 *    write fails, that client should be disconnected, but it may not be easy to
 *    remove the client immediately. This can be local to main, but will then
 *    need to be passed to any functions that need access to it.
 * 
 * 2) Consider making a function that does the accept and inserts the new file
 *    descriptor into the client list.
 * 
 * 3) Consider making a function to clean up dead connections from the
 *    connection list, or do a cleanup pass before entering back into select.
 * 
 *    Both adding and removing connections should update a saved read fd_set
 *    which is copied to a working fd_set before select. Both should also re-
 *    compute the highest working file-descriptor number (+1) for selects first
 *    parameter.
 * 
 * 4) Consider making a function to broadcast the received message to all the
 *    currently connected clients, but not to the originator of the message.
 *    If any of the writes should fail, that client should be marked dead or
 *    removed from the client list.
 * 
 * 5) The main function should create and bind a streaming internet socket on to
 *    listen for connections on your assigned port.  It should initialize the
 *    client list and initialize a saved fd_set with just the socket descriptor.
 *    It should then enter an infinite loop and use select to wait for read
 *    events.  If the read event is for the listening socket it should accept
 *    the connection, otherwise it is a client message that should be broadcast
 *    to all other clients.
 */

int handleRead(int fd) {
  char buf[K];
  int r, w, t;

 r = read(fd, buf, K);
  if (r < 0)
	  perror("read");
  else if (r == 0)
	  return -1;
  else {
	  for (int i = 4; i < FD_SETSIZE; i++) {
		  if (FD_ISSET(i, &active)) {
	    if (i != fd) {
	     t = 0;
	    do {
	      w = write(i, buf+t, r-t);
	      t += w;
	    } while (t < r);
	    }
	}
	  }
	 
	  return 0;
  }
}

void markDead(int fd) {
 close(fd);
 FD_CLR(fd, &active);

}

int main(int argc, char *argv[])
{


  int sock = socket(AF_INET, SOCK_STREAM, 0);


  int option = 1;
  if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *)&option, sizeof(option)) < 0)
	  perror("setsockopt");


  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);

  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr *) &addr, sizeof(struct sockaddr_un)) < 0) {
    perror("bind");
    exit(1); 
  }


  if (listen(sock, 128) < 0) {
    perror("listen");
    exit(0);
  }

  FD_ZERO(&active);
  FD_SET(sock, &active);

  for(;;) {
  
   rset = active;
   int count = 0;
  if ((count = select(FD_SETSIZE, &rset, NULL, NULL, NULL)) < 0)
    perror("select");

  for (int i = 0; i < FD_SETSIZE; i++)
	  if (FD_ISSET(i, &rset)) {
	    if (i == sock) {
		    socklen_t client_len = sizeof(struct sockaddr_in);
		    struct sockaddr_in client_addr;
	            
		    int client_sock = accept(sock, (struct sockaddr*) &client_addr, &client_len); 

		    if (client_sock < 0)
			    perror("accept");
		  fprintf(stderr, "Server: Connection from descriptor %d on address %s\n", client_sock, inet_ntoa(client_addr.sin_addr));

		    FD_SET(client_sock, &active);
	    } else {
	      if (handleRead(i) < 0) {
	       markDead(i); 
	      }
	    }
	  
	  }
  
  }

  return 0;
}
