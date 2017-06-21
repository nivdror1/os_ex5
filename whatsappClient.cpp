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

std::string nickname;

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
	if(remainingMessageIndex < text.size()){
		tokens.push_back(text.substr(remainingMessageIndex));
	}

    return tokens;
}

bool isNotAlphaNumeric(std::string &name){
    auto compare = [](char c) { return !(isalnum(c)); };
    return std::find_if(name.begin(), name.end(),compare) != name.end();
}

/**
 * init
 */
void init(int clientSocket, char* argv[]){
	//initiate the fd set listeningFd
	FD_ZERO(&listeningFds);
	FD_SET(clientSocket,&listeningFds);
	FD_SET(STDIN_FILENO,&listeningFds);

    nickname = argv[1];
    if (isNotAlphaNumeric(nickname)){
        // todo usage error
    }
    unsigned char buf[sizeof(struct in6_addr)];
    if (inet_pton(AF_INET, argv[2], buf) == 0){
        // todo usage error (not an IP address)
    }

	//init the struct sockaddr_in
	memset(&sa, 0, sizeof(struct sockaddr_in));//todo check if memset can fail
	sa.sin_family= AF_INET;
	sa.sin_addr.s_addr = inet_addr(argv[2]);
	unsigned short portnum = (unsigned short)atoi(argv[3]);
	sa.sin_port = htons(portnum);
}

std::string parseCreateGroup(std::string &message){

	std::vector<std::string> splitMessage = split(message, ' ', 2);
    // if splitMessage.size() == 2 that is format error
	if (splitMessage.size() != 2){
		// todo usage error
	}
	std::string groupName = splitMessage.at(0);

	if (isNotAlphaNumeric(groupName)){
		//todo usage error
	}
	std::vector<std::string> groupMembers = split(splitMessage.at(1), ',', 30);
    // if groupMembers.size() > 30 then there is more than 30 group members and that is format error
	if (groupMembers.size() > 30){
		// todo usage error
	}
	if (std::find_if(groupMembers.begin(), groupMembers.end(),isNotAlphaNumeric) != groupMembers.end()){
		// todo usage error
	}
	std::string groupMembersWithSpaces;
	for (unsigned int i = 0; i < groupMembers.size()-1; i++){
		groupMembersWithSpaces += groupMembers.at(i) + " ";
	}
	groupMembersWithSpaces += groupMembers.at(groupMembers.size()-1);
	return groupName + " " + groupMembersWithSpaces;
}

std::string parseSendCommand(std::string &message){
    std::vector<std::string> splitMessage = split(message, ' ', 1);
    // if splitMessage.size() <= 1 then receiver name or the actual message is missing
    if (splitMessage.size() <= 1){
        // todo usage error
    }
    std::string receiverName = splitMessage.at(0);

    if (isNotAlphaNumeric(receiverName) || receiverName == nickname){
        //todo usage error
    }
    return receiverName + " " + splitMessage.at(1);
}

void handleRequestFromUser(int clientSocket){
	char buf[MAX_CHAR];
	if (read(STDIN_FILENO, buf, MAX_CHAR) < 0){
		//todo error
	}
	std::vector<std::string> splitMessage = split(buf, ' ', 1);
	auto found = splitMessage.at(0).find_first_of("\n");
//	if(found!= std::string::npos){
//		splitMessage.at(0) = splitMessage.at(0).substr(0, found);
//	}
	std::string messageToServer = splitMessage.at(0);
    if(splitMessage.at(0) == "create_group"){
	    size_t messageLength = splitMessage.at(1).find_last_of("\n");
	    std::string messageWithoutEndl = splitMessage.at(1).substr(0, messageLength);
		messageToServer += " " + parseCreateGroup(messageWithoutEndl);
	}else if(splitMessage.at(0) == "send"){
	    size_t messageLength = splitMessage.at(1).find_last_of("\n");
	    std::string messageWithoutEndl = splitMessage.at(1).substr(0, messageLength);
        messageToServer += " " + parseSendCommand(messageWithoutEndl);
	}else if(splitMessage.at(0).substr(0,found) == "who" || splitMessage.at(0).substr(0,found) == "exit"){
        if (splitMessage.size() != 1){
            // todo usage error
        }
	}else{
		//todo usage error
	}
    if(send(clientSocket,messageToServer.c_str(),messageToServer.size(),0) != (ssize_t)messageToServer.size()){
        //todo error
    }
}

void checkIfShouldTerminate(const char* message){
    if (strcmp(message, "Client name is already in use.\n") == 0 ||
        strcmp(message, "Failed to connect the server.\n") == 0){
        std::cerr << message;
        exit(1);
    }
    if (strcmp(message, "Shut down server.\n") == 0){
        exit(1);
    }
    if (strcmp(message, "Unregistered successfully.\n") == 0){
        std::cout << message;
        exit(0);
    }
}

void getMessageFromServer(int clientSocket){
	char buf[400];
	memset(buf, '0', sizeof(buf));
	ssize_t numberOfBytesRead = read(clientSocket, buf, MAX_CHAR);
	if(numberOfBytesRead < 0){
		//todo error
	}
	std::string text = buf;
	text = text.substr(0,(size_t)numberOfBytesRead);
    checkIfShouldTerminate(text.c_str());

	std::cout << text;
}

void wakeUpClient(fd_set &readFds, int clientSocket){
	if(FD_ISSET(STDIN_FILENO,&readFds)){
		handleRequestFromUser(clientSocket);
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
	init(clientSocket,argv);
	//connect to the main server socket
	if (connect(clientSocket , (struct sockaddr *)&sa , sizeof(sa)) < 0) {
		close(clientSocket );//todo error close can fail
		return(-1);
	}
    if(send(clientSocket, argv[1],strlen(argv[1]) + 1,0) != (ssize_t)strlen(argv[1]) + 1){
        //todo error
    }
	int ready = 0;
	while(true){
		fd_set readFds = listeningFds;
		if((ready = select(clientSocket+1, &readFds, NULL, NULL,NULL))<0){
			//todo error
			return -1;
		}
		if(ready > 0){
			wakeUpClient(readFds, clientSocket);
		}

	}

}