#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define QUEUE_LENGTH 10
#define MAX_DATA_SIZE 100
#define PORT "1234"

using namespace std;

int main() {
	int sockfd;
	struct addrinfo hints, *servinfo, *ptr;
	struct sockaddr_storage incomingAddr;

	socklen_t incomingSize;
	char s[INET6_ADDRSTRLEN];
	int rv;

	char incomingStr[MAX_DATA_SIZE];
	int yes = 1;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // Limit to IPv4 addresses only
	hints.ai_socktype = SOCK_STREAM; // Define TCP socket
	hints.ai_flags = AI_PASSIVE; // Serve from the local machine's IP address

	rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
	if (rv != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(rv);
		return 1;
	}

	
	for (ptr = servinfo; ptr != NULL; ptr = ptr->ai_next) {
		// Creating the socket connection using the derived addrinfo struct and ensuring that it is valid
		sockfd = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (sockfd == -1) {
			perror("server: socket");
			continue;
		}

		// Configure socket to allow other active non-listening sockets to bind() to this port
		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("server: setsockopt");
			return 1;
		}

		if (bind(sockfd, ptr->ai_addr, ptr->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind()");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // Socket has been configured so we no longer need metadata about the client
	
	if (ptr == NULL) {
		std::cerr << "server: no compatible addresses to bind to" << std::endl;
		return 1;
	}

	if (listen(sockfd, QUEUE_LENGTH) == -1) {
		perror("listen");
		return 1;
	}

	std::cout << "server: waiting for connection..." << std::endl;

	incomingSize = sizeof incomingAddr;
	int incomingfd = accept(sockfd, (struct sockaddr *)&incomingAddr, &incomingSize);
	if (incomingfd == -1) {
		perror("accept");
		return 2;
	}
	
	// Converting the incoming IP address from pure binary to a human-readable format
	inet_ntop(incomingAddr.ss_family, (struct sockaddr *)&incomingAddr, s, sizeof s);
	std::cout << "server: receieved connection from " << s << std::endl;

	while (1) {
		incomingStr[0] = '\0'; 

		if (recv(incomingfd, incomingStr, sizeof incomingStr, 0) == -1) {
			perror("recv");
			continue;
		}

		std::cout << "server: receieved content " << incomingStr << std::endl;

		if ((send(incomingfd, "OK", 4, 0)) == -1) {
			perror("send");
			continue;
		}
	
		
		if (!fork()) {
			close(sockfd);
			if (send(incomingfd, "closing connection...", 25, 0) == -1) {
				perror("send");
			}
			close(incomingfd);
			exit(0);
		}

	}

}
