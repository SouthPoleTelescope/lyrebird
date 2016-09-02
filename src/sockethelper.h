#pragma once
#include <vector>
#include <string>
int initialize_socket_connection(const char * hostname, int portno, 
				 int timeout_us, int & sockfd);
int get_string_list(int sockfd, std::vector<std::string> & strs);
int bind_udp_socket(int & sockfd, const char * hostname, int portno);
