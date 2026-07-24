#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unordered_map>
#include <vector>
#include <sstream>
#include <cctype>

#define QUEUE_LENGTH 10
#define MAX_DATA_SIZE 100
#define PORT "1234"

using namespace std;

enum Command {
	SET,
	GET,
	DELETE,
	EXIT
};

void tokenizeBySpaces(const std::string &input, std::vector<std::string> &tokens) {
	istringstream iss(input);
	std::string buf;

	while (getline(iss, buf, ' ')) {
		tokens.push_back(buf);
	}

	return;
}

//std::string setCmd(string key, string value, std::unordered_map<string, string> &items) {

//}

//std::string getCmd(string key, std::unordered_map<string, string> &items) {

//}

//std::string delCmd(string key, std::unordered_map<string, string> &items) {

//}


int main() {
	int sockfd;
	ssize_t numBytes;
	struct addrinfo hints, *servinfo, *ptr;
	struct sockaddr_storage incomingAddr;

	socklen_t incomingSize;
	char s[INET6_ADDRSTRLEN];
	int rv;

	char buf[MAX_DATA_SIZE];
	char incomingStr[MAX_DATA_SIZE];
	int yes = 1;

	std::string pending;
	char errCmd[] = "ERROR: send messages in the form {COMMAND KEY VALUE}\ne.g. SET name dhruv\n";
	char keyNotFoundErr[] = "ERROR: requested key not found in store";


	std::vector<std::string> tokens;
	std::unordered_map<string, string> pairs;

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

	std::string cmd;
	std::string key;
	std::string value;

	while (1) {
		numBytes = recv(incomingfd, buf, sizeof buf, 0);

		if (numBytes == -1) {
			perror("server: recv");
			continue;
		}

		if (numBytes == 0) {
			std::cout << "server: client disconnected" << std::endl;
			break;
		}

		pending.append(buf, numBytes);
		std::size_t newlinePos;
		bool shouldBreak = false;

		// Commands are newline-delimited to avoid issues with TCP latency breaking up commands
		while ((newlinePos = pending.find('\n')) != std::string::npos) {
			std::string command = pending.substr(0, newlinePos);
			pending.erase(0, newlinePos+1);

			if (command == "exit") {
				if (send(incomingfd, "closing connection...", 25, 0) == -1) {
					perror("send");
				}

				close(incomingfd);
				shouldBreak = true;
				break;
			}

			tokenizeBySpaces(command, tokens);
			std::cout << "server: receieved command " << command << '\n';

			if (tokens.size() < 2) {
				if ((send(incomingfd, errCmd, strlen(errCmd), 0)) == -1) {
					perror("send");
					continue;
				}

				tokens.clear();
				break;
			}

			cmd = tokens[0];
			key = tokens[1];

			if (tokens.size() == 2) {
			    std::cout << "CMD: " << cmd << "  KEY: " << key << std::endl;

				// TODO: Figure out how to normalize cmd to lowercase
				if (cmd == "GET") {
				    try {

                        std::string associatedVal = pairs.at(key);
                        std::string getResponse = "Key: " + key + "\tValue: " + associatedVal;
                        if (send(incomingfd, getResponse.data(), getResponse.size(), 0) == -1) {
                            perror("server: send");
                        }

					} catch (std::out_of_range) {

					    if (send(incomingfd, keyNotFoundErr, sizeof keyNotFoundErr, 0) == -1) {
							perror("server: send key not found error");
							break;
						}

					}
				} else if (cmd == "DELETE") {

                    std::size_t erased = pairs.erase(key);

                    if (erased == 0) {
                        if (send(incomingfd, keyNotFoundErr, sizeof keyNotFoundErr, 0) == -1) {
                            perror("server: send key not found error");
                            break;
                        }
                    } else if (erased == 1) {
                        std:string deleteMsg = "successfully deleted entry with key " + key;

                        if (send(incomingfd, deleteMsg.data(), deleteMsg.size(), 0) == -1) {
                            perror("server: send key not found error");
                            break;
                        }
                    }

				} else {
				    // TODO: send this out instead of just printing it on server side
				    std::cout << "ERROR: NOT A COMMAND" << std::endl;
				}

			} else {
                for (int i = 2; i < tokens.size(); i++) {
                    value.append(tokens[i]);
                    value.append(" ");
                }

                std::cout << "CMD: " << cmd << "  KEY: " << key << "  VAL: " << value << std::endl;

                if (cmd == "SET") {
                    // check if the key already exists in the entries and just return that if it is
                    try {
                         std::string associatedVal = pairs.at(key);

                         std::string getResponse = "Your entry already exists with key: " + key + " and value: " + associatedVal;
                         if (send(incomingfd, getResponse.c_str(), sizeof getResponse, 0) == -1) {
                             perror("server: send setResponse (key found)");
                         }

                    } catch (std::out_of_range) {
                        pairs.insert({key, value});
                        std::string insertMsg = "Entry with key: " + key + " and value: " + value + " has been inserted successfully";

                        if (send(incomingfd, insertMsg.data(), insertMsg.size(), 0) == -1) {
                            perror("server: send setResponse (key found)");
                        }
                    }
                } else {
                    // TODO: send this back to the client
                    std::cout << "ERROR: NOT A COMMAND" << std::endl;
                }
			}

			// if ((send(incomingfd, "OK\n", 3, 0)) == -1) {
			// 	perror("send");
			// 	continue;
			// }

			cmd.clear();
			key.clear();
			value.clear();
			tokens.clear();
		}

		if (shouldBreak) {
			break;
		}
	}

	close(sockfd);

	return 0;

}
