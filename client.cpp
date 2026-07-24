#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string>

#define QUEUE_LENGTH 10
#define MAX_DATA_SIZE 100
#define PORT "1234"

using namespace std;

int main(int argc, char *argv[]) {
	int sockfd, numBytes;
	struct addrinfo hints, *servinfo, *p;
	char buf[MAX_DATA_SIZE];
	int rv;
	char s[INET6_ADDRSTRLEN];

	std::string strToSend;

	if (argc != 2) {
		std::cerr << "usage: client hostname" << std::endl;
		exit(1);
	}
	
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	rv = getaddrinfo(argv[1], PORT, &hints, &servinfo);
	if (rv != 0) {
		std::cerr << "getaddrinfo: " << gai_strerror(rv);
		return 1;
	}

	for (p = servinfo; p != NULL; p = p->ai_next) {
		sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (sockfd == -1) {
			perror("client: socket");
			return 1;
		}

		inet_ntop(p->ai_family, p->ai_addr, s, sizeof s);

		std::cout << "client: attempting connection to: " << s << std::endl;

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		std::cerr << "client: failed to connect" << std::endl;
		return 2;
	}

	inet_ntop(p->ai_family, p->ai_addr, s, sizeof s);
	std::cout << "client: connected to " << s << std::endl;
	
	// master loop
	while (1) {
		std::cout << "prompt> ";
		//std::cin >> strToSend;
		//std::getline(std::cin, strToSend);

		if (!std::getline(std::cin, strToSend)) {
			break;
    		}

		if (send(sockfd, strToSend.data(), strToSend.size(), 0) == -1) {
			perror("client: send");
			continue;
		}
		
		if ((numBytes = recv(sockfd, buf, sizeof buf, 0)) == -1) {
			perror("client: receive");
			continue;
		}
		
		if (numBytes == 0) {
			std::cout << "client: server disconnected" << std::endl;
			break;
		}
		
//		std::cout << "sent content: " << strToSend << std::endl;
		std::cout.write(buf, numBytes);
		std::cout << std::endl;
	}

	close(sockfd);

	return 0;
}
