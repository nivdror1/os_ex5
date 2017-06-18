#include <stdlib.h>

#include <netdb.h>
#include <map>
#include <vector>
#include <cstring>

#define MAX_CLIENT 10

/** a map from a client name to a sockId*/
std::map<char *, int> sockIdentifier;

/** a map from a group name to a vector of client names in said group*/
std::map<char* , std::vector<char*>> groupMembers;

int init(){
	return 0;
}

int finalizer(){
	return 0;
}


int main(int argc, char *argv[]){
	struct hostent *h;
	struct sockaddr_in sa;

	if(argc != 2){
		//todo error
		return -1;
	}
	//get the host name(server port)
	if((h=gethostbyname(argv[1])) == NULL){
		//todo error
		return -1;
	}
	//create a new socket
	int sockFD = socket(AF_INET,SOCK_STREAM,0);
	if(sockFD == -1){
		//todo error
		return -1;
	}

	memset(&sa, 0, sizeof(struct sockaddr));//todo check if memset can fail
	sa.sin_family= (sinfamily_t)h->h_addrtype;
	memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
	return 0;

}


