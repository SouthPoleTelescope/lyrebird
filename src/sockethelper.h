#pragma once
#include <vector>
#include <string>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>

int fill_addr(const char * hostname, int portno, struct sockaddr_in & addr);
int initialize_socket_connection(const char * hostname, int portno, 
				 int timeout_us, int & sockfd);
int get_string_list(int sockfd, std::vector<std::string> & strs);
int bind_udp_socket(int & sockfd, const char * hostname, int portno);
