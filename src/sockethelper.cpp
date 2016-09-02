#include "sockethelper.h"

#include <stdlib.h>
#if defined(__linux__)
#  include <endian.h>
#elif defined(__APPLE__)
#   include <libkern/OSByteOrder.h>

#   define htobe16(x) OSSwapHostToBigInt16(x)
#   define htole16(x) OSSwapHostToLittleInt16(x)
#   define be16toh(x) OSSwapBigToHostInt16(x)
#   define le16toh(x) OSSwapLittleToHostInt16(x)

#   define htobe32(x) OSSwapHostToBigInt32(x)
#   define htole32(x) OSSwapHostToLittleInt32(x)
#   define be32toh(x) OSSwapBigToHostInt32(x)
#   define le32toh(x) OSSwapLittleToHostInt32(x)

#   define htobe64(x) OSSwapHostToBigInt64(x)
#   define htole64(x) OSSwapHostToLittleInt64(x)
#   define be64toh(x) OSSwapBigToHostInt64(x)
#   define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(__FreeBSD__) || defined(__NetBSD__)
#  include <sys/endian.h>
#elif defined(__OpenBSD__)
#  include <sys/types.h>
#  define be16toh(x) betoh16(x)
#  define be32toh(x) betoh32(x)
#  define be64toh(x) betoh64(x)
#endif

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <streambuf>
#include <vector>
#include <string>

int connect_with_timeout(int sockfd, struct sockaddr * addr, socklen_t addrlen, 
			 unsigned int timeout_us){
	int res;
	long arg;
	struct timeval tv; 
	fd_set myset; 
	int valopt; 
	socklen_t lon; 
	
	//set to non blocking
	if( (arg = fcntl(sockfd, F_GETFL, NULL)) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		return -1;
	} 
	
	arg |= O_NONBLOCK;
	
	if( fcntl(sockfd, F_SETFL, arg) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		return -1;
	} 
	
	//try to connect
	res = connect(sockfd, addr, addrlen ); 
	
	if (res < 0) { 
		if (errno == EINPROGRESS) { 
			//fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
			tv.tv_sec = 0; 
			tv.tv_usec = timeout_us; 
			FD_ZERO(&myset); 
			FD_SET(sockfd, &myset); 
			res = select(sockfd+1, NULL, &myset, NULL, &tv); 
			if (res < 0 && errno != EINTR) { 
				fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
				return -1;
			} 
			else if (res > 0) { 
				// Socket selected for write 
				lon = sizeof(int); 
				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
					fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
					return -1;
				} 
				// Check the value returned... 
				if (valopt) { 
					fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) 
						); 
					return -1;
				} 
			} 
			else { 
				fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
				return -1;
			} 
		} 
		else { 
			fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
			return -1;
		} 
	} 
	// Set to blocking mode again... 
	if( (arg = fcntl(sockfd, F_GETFL, NULL)) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_GETFL) (%s)\n", strerror(errno)); 
		return -1;
	} 
	arg &= (~O_NONBLOCK); 
	if( fcntl(sockfd, F_SETFL, arg) < 0) { 
		fprintf(stderr, "Error fcntl(..., F_SETFL) (%s)\n", strerror(errno)); 
		return -1;
	} 
	return 0;
}


int fill_addr(const char * hostname, int portno, struct sockaddr_in & addr) {
	struct hostent *server;
	server = gethostbyname(hostname);
	if (server == NULL){
		perror("ERROR, no such host");
		return 1;
	}
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portno);
	memcpy(&addr.sin_addr.s_addr,server->h_addr,server->h_length);
	return 0;
}



int bind_udp_socket(int & sockfd, const char * hostname, int portno) {
	struct sockaddr_in serv_addr;
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { 
		perror("cannot create socket"); return -1; 
		
	}
	if (fill_addr(hostname, portno, serv_addr)){
		perror("ERROR getting hostname");
		goto cleanup_connect;
	}
	if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { 
		perror("bind failed");
		goto cleanup_connect;
	}

	return 0;
cleanup_connect:
	close(sockfd);
	return 1;

}



int initialize_socket_connection(const char * hostname, int portno, int timeout_us, int & sockfd) {
	int return_val = 1;		
	struct sockaddr_in serv_addr;
	int bytes, sent, received, total, loc, end_found, content_len;
	char header[1024]; //has magic number of 1024 below
	char * content_len_str = NULL;
	//char * content_buffer = NULL;
		
	/* create the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		perror("ERROR opening socket");
		return 1;
	}

	if (fill_addr(hostname, portno, serv_addr)){
		perror("ERROR getting hostname");
		goto cleanup_connect;
	}

	/* connect the socket */
	if (connect_with_timeout(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr), timeout_us) < 0){
		perror("ERROR connecting");
		goto cleanup_connect;
	}
	return 0;
cleanup_connect:
	close(sockfd);
	return 1;
}



int get_string_list(int sockfd, std::vector<std::string> & strs) {
	#define BUFSIZE 1024
	char buf[BUFSIZE]; 
	ssize_t recvlen = 0;
	strs.clear();
	do {
		recvlen = recv(sockfd, buf, BUFSIZE-1, MSG_DONTWAIT);
		if (recvlen>0){
			buf[recvlen] = '\0';
			strs.push_back(std::string(buf));
		}
	} while (recvlen > 0);
	return 0;
}


