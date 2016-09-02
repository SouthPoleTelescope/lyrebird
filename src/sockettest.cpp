#include "sockethelper.h"
#include <stdio.h>
int main(){
	int sockfd;
	int is_bad =  bind_udp_socket(sockfd, "127.0.0.1", 5555);
	if (!is_bad) {
		printf("good\n");
	} else {
		printf("bad\n");
	}
	std::vector<std::string> strs;
	while (1) {
		get_string_list(sockfd, strs);
		for (size_t i=0; i <strs.size(); i++) {
			printf("%s\n", strs[i].c_str());
		}
	}


	return 0;
}
