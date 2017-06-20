#include <stdlib.h>

#include <netdb.h>
#include <map>
#include <vector>
#include <cstring>
#include <zconf.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <vector>
#include <set>
#include <arpa/inet.h>

#define MAX_CHAR 400

fd_set listeningFds;

struct sockaddr_in sa;


/**
 * init
 */
void init(int clientSocket, char* address,char* port){
	//initiate the fd set listeningFd
	FD_ZERO(&listeningFds);
	FD_SET(clientSocket,&listeningFds);
	FD_SET(STDIN_FILENO,&listeningFds);

	//init the struct sockaddr_in
	memset(&sa, 0, sizeof(struct sockaddr_in));//todo check if memset can fail
	sa.sin_family= AF_INET;
	sa.sin_addr.s_addr = inet_addr(address);
	unsigned short portnum = (unsigned short)atoi(port);
	sa.sin_port = htons(portnum);
}

std::vector<std::string> split(const std::string &text, char delim, int counter) {
	std::stringstream textStream(text);
	std::string item;
	std::vector<std::string> tokens;
	unsigned int remainingMessageIndex = 0;
	while (getline(textStream, item, delim)) {
		tokens.push_back(item);
		remainingMessageIndex += (item.size() + 1);
		counter--;
		if(counter == 0){
			break;
		}
	}
	tokens.push_back(text.substr(remainingMessageIndex));
	return tokens;
}

bool isAlphaNumeric(std::string &name){
	auto compare = [](char c) { return !(isalnum(c)); };
	return std::find_if(name.begin(), name.end(),compare) == name.end();
}

std::string parseCreateGroup(std::string &message){

	std::vector<std::string> splitMessage = split(message, ' ', 2);
	if (splitMessage.size() > 2){
		// todo usage error
	}
	std::string groupName = splitMessage.at(0);

	if (!isAlphaNumeric(groupName)){
		//todo usage error
	}
	std::vector<std::string> groupMembers = split(message, ',', 30);
	if (groupMembers.size() > 30){
		// todo usage error
	}
	if (std::find_if(groupMembers.begin(), groupMembers.end(),isAlphaNumeric) != groupMembers.end()){
		// todo usage error
	}
	std::string groupMembersWithSpaces;
	for (unsigned int i = 0; i < groupMembers.size()-1; i++){
		groupMembersWithSpaces += groupMembers.at(i) + " ";
	}
	groupMembersWithSpaces += groupMembers.at(groupMembers.size()-1);
	return groupMembersWithSpaces;
}

void handleRequestFromUser(){
	char buf[MAX_CHAR];
	if (read(STDIN_FILENO, buf, MAX_CHAR) < 0){
		//todo error
	}
	std::vector<std::string> splitMessage = split(buf, ' ', 1);
	std::string messageToServer;
	if(splitMessage.at(0) == "create_group"){
		messageToServer = parseCreateGroup(splitMessage.at(1));
	}else if(splitMessage.at(0) == "send"){

	}else if(splitMessage.at(0) == "who"){

	}else if(splitMessage.at(0) == "exit") {

	}else{
		//todo usage error
	}
}

void getMessageFromServer(int clientSocket){
	char buf[400];
	memset(buf, '0', sizeof(buf));
	if(read(clientSocket, buf, MAX_CHAR)< 0){
		//todo error
	}
	std::cout << buf;
}

void wakeUpClient(fd_set &readFds, int clientSocket){
	if(FD_ISSET(STDIN_FILENO,&readFds)){
		handleRequestFromUser();
	}else{
		//receive and send messages from/to the client
		getMessageFromServer(clientSocket);
	}
}

int main(int argc, char *argv[]){

	if(argc != 4){
		//todo error
		return -1;
	}
	//create a new socket
	int clientSocket = socket(AF_INET, SOCK_STREAM,0);
	if(clientSocket == -1){
		//todo error
		return -1;
	}

	init(clientSocket,argv[2], argv[3]);
	//connect to the main server socket
	if (connect(clientSocket , (struct sockaddr *)&sa , sizeof(sa)) < 0) {
		close(clientSocket );//todo error close can fail
		return(-1);
	}

	int ready = 0;
	while(true){
		fd_set readFds = listeningFds;
		if((ready = select(2, &readFds, NULL, NULL,NULL))<0){
			//todo error
			return -1;
		}
		if(ready > 0){
			wakeUpClient(readFds, clientSocket);
		}

	}

}