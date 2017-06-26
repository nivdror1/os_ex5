#include <stdlib.h>

#include <netdb.h>
#include <map>
#include <vector>
#include <cstring>
#include <zconf.h>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <set>
#include <arpa/inet.h>

/* Maximum number of characters in message include protocol format. */
#define MAX_CHAR 400

/* File descriptors' set, contain std::cin and client socket. */
fd_set listeningFds;

struct sockaddr_in sa;

/* The name of this client. */
std::string nickname;

/* The client socket. */
int clientSocket;

/**
 * @brief splits the text counter times according to delimiter parameter.
 * @param text the text to split
 * @param delim the delimiter to split according to.
 * @param counter number of times to split
 * @return vector of tokens from this text
 */
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

/**
 * @brief returns true if the given input contains characters that aren't alphanumeric
 * @param name the name to check
 * @return true if the given input contains characters that aren't alphanumeric, false otherwise
 */
bool isNotAlphaNumeric(std::string &name){
    auto compare = [](char c) { return !(isalnum(c)); };
    return std::find_if(name.begin(), name.end(),compare) != name.end();
}

/**
 * init
 */
void init(char* argv[]){
	//initiate the fd set listeningFd
	FD_ZERO(&listeningFds);
	FD_SET(clientSocket,&listeningFds);
	FD_SET(STDIN_FILENO,&listeningFds);

    nickname = argv[1];
    if (isNotAlphaNumeric(nickname)){
        std::cerr<< "Failed to connect the server.\n";
        exit(1);
    }
    unsigned char buf[sizeof(struct in6_addr)];
    memset(buf, '0', sizeof(buf));
    if (inet_pton(AF_INET, argv[2], buf) == 0){
        std::cerr<< "Failed to connect the server.\n";
        exit(1);
    }

	//init the struct sockaddr_in
	memset(&sa, 0, sizeof(struct sockaddr_in));
	sa.sin_family= AF_INET;
	sa.sin_addr.s_addr = inet_addr(argv[2]);
	unsigned short portnum = (unsigned short)atoi(argv[3]);
	sa.sin_port = htons(portnum);
}

/**
 * @brief parses the given create group message, checks if the input is valid
 * @param message create group message
 * @return if the message is valid returns string of the tokens of create group message splits by
 * whitespace, empty string otherwise.
 */
std::string parseCreateGroup(std::string &message){

	std::vector<std::string> splitMessage = split(message, ' ', 2);
    // if splitMessage.size() == 2 that is format error
	if (splitMessage.size() != 2){
        if(splitMessage.size() == 0){
            std::cerr<< "ERROR: failed to create group \"\".\n";
        }else{
            std::cerr<< "ERROR: failed to create group \""+splitMessage.at(0)+"\".\n";
        }
        return {};
	}
	std::string groupName = splitMessage.at(0);
	std::vector<std::string> groupMembers = split(splitMessage.at(1), ',', 30);

	// if first condition is true, then that is one man group, and this is error
	// if groupMembers.size() > 30 then there is more than 30 group members and that is format error
	if((groupMembers.size() == 1 && groupMembers.at(0) == nickname) || groupMembers.size() > 30 ){
        std::cerr<< "ERROR: failed to create group \""+ groupName +"\".\n";
        return {};
	}
	if (isNotAlphaNumeric(groupName) || std::find_if(groupMembers.begin(), groupMembers.end(),
												  isNotAlphaNumeric) != groupMembers.end())
    {
        std::cerr << "ERROR: failed to create group \"" + groupName + "\".\n";
        return {};
    }
	std::string groupMembersWithSpaces;
	for (unsigned int i = 0; i < groupMembers.size()-1; i++){
		groupMembersWithSpaces += groupMembers.at(i) + " ";
	}
	groupMembersWithSpaces += groupMembers.at(groupMembers.size()-1);
	return groupName + " " + groupMembersWithSpaces;
}

/**
 * @brief parses the given create group message, checks if the input is valid
 * @param message create group message
 * @return if the message is valid returns string of the tokens of create group message splits by
 * whitespace, empty string otherwise.
 */
std::string parseSendCommand(std::string &message){
    std::vector<std::string> splitMessage = split(message, ' ', 1);
    // if splitMessage.size() <= 1 then receiver name or the actual message is missing
    if (splitMessage.size() <= 1){
        std::cerr<< "ERROR: failed to send.\n";
        return {};
    }
    std::string receiverName = splitMessage.at(0);

    if (isNotAlphaNumeric(receiverName) || receiverName == nickname){
        std::cerr<< "ERROR: failed to send.\n";
        return {};
    }
    return receiverName + " " + splitMessage.at(1);
}

/**
 * @brief Gets message from user, if the message is valid send it to the server, otherwise print
 * error message.
 */
void handleRequestFromUser(){
	char buf[MAX_CHAR];
    memset(buf, '0', sizeof(buf));
    std::string groupText,sendText;
    ssize_t numberOfBytes = read(STDIN_FILENO, buf, MAX_CHAR);
	if (numberOfBytes < 0){
        std::cerr << "ERROR: read " << errno << ".\n";
        exit(1);
	}
    std::string buffString(buf);
	std::vector<std::string> splitMessage = split((buffString.substr(0, (size_t)numberOfBytes))
                                                          .c_str(), ' ', 1);
	auto found = splitMessage.at(0).find_first_of("\n");
	std::string messageToServer = splitMessage.at(0);
    if(splitMessage.at(0) == "create_group"){
	    size_t messageLength = splitMessage.at(1).find_last_of("\n");
	    std::string messageWithoutEndl = splitMessage.at(1).substr(0, messageLength);
        if((groupText =parseCreateGroup(messageWithoutEndl)).empty()){
            return;
        }
		messageToServer += " " + groupText;
	}else if(splitMessage.at(0) == "send"){
	    size_t messageLength = splitMessage.at(1).find_last_of("\n");
	    std::string messageWithoutEndl = splitMessage.at(1).substr(0, messageLength);
        if((sendText =parseSendCommand(messageWithoutEndl)).empty()){
            return;
        }
        messageToServer += " " + sendText;
	}else if(splitMessage.at(0).substr(0,found) == "who"){
        if (splitMessage.size() != 1){
            std::cerr<< "ERROR: failed to receive list of connected clients.\n";
            return;
        }
    }else if(splitMessage.at(0).substr(0,found) == "exit"){
        if (splitMessage.size() != 1){
            std::cerr<< "ERROR: failed to exit.\n";
            return;
        }
	}else{
        std::cerr<< "ERROR: Invalid input.\n";
        return;
	}
    if(send(clientSocket,messageToServer.c_str(),messageToServer.size(),0) != (ssize_t)messageToServer.size()){
        std::cerr << "ERROR: send " << errno << ".\n";
        exit(1);
    }
}

/**
 * @brief checks the message from the server, exit from server if it is terminate message according
 * to communication protocol.
 * @param message
 */
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

/**
 * @brief Gets message from server, checks if this client should terminate and prints accordingly.
 */
void getMessageFromServer(){
	char buf[400];
	memset(buf, '0', sizeof(buf));
	ssize_t numberOfBytesRead = read(clientSocket, buf, MAX_CHAR);
	if(numberOfBytesRead < 0){
        std::cerr << "ERROR: read " << errno << ".\n";
        exit(1);
	}
	std::string text = buf;
    //check if it's an error
    if(text.substr(0,5)=="ERROR"){
        std::cerr<<text.substr(0,(size_t)numberOfBytesRead);
        return;
    }
	text = text.substr(0,(size_t)numberOfBytesRead);
    checkIfShouldTerminate(text.c_str());

	std::cout << text;
}

/**
 * @brief checks if the message come from the user or the server and acts accordingly.
 * @param readFds
 */
void wakeUpClient(fd_set &readFds){
	if(FD_ISSET(STDIN_FILENO,&readFds)){
		handleRequestFromUser();
	}else{
		//receive and send messages from/to the client
		getMessageFromServer();
	}
}

/**
 * @brief Gets client name as an input and try to connect and send it to the server.
 * @param clientName the name of this client
 */
void connectToServer(char* clientName){
	//connect to the main server socket
	if ( (connect(clientSocket , (struct sockaddr *)&sa , sizeof(sa))) < 0) {
        std::cerr << "ERROR: connect " << errno << ".\n";
		if(close(clientSocket )){
            std::cerr << "ERROR: close " << errno << ".\n";
        }
        exit(1);
	}
    if((send(clientSocket, clientName,strlen(clientName) + 1,0)) != (ssize_t)strlen(clientName) + 1){
        std::cerr << "ERROR: send " << errno << ".\n";
        exit(1);
    }
}

int main(int argc, char *argv[]){

	if(argc != 4){
        std::cerr << "Usage: whatsappClient clientName serverAddress serverPort" << std::endl;
        exit(1);
	}
	//create a new socket
	if((clientSocket = socket(AF_INET, SOCK_STREAM,0)) == -1){
        std::cerr << "ERROR: socket " << errno << ".\n";
        exit(1);
	}
	init(argv);
    connectToServer(argv[1]);
	int ready = 0;
	while(true){
		fd_set readFds = listeningFds;
		if((ready = select(clientSocket+1, &readFds, NULL, NULL,NULL))<0){
            std::cerr << "ERROR: select " << errno << ".\n";
            exit(1);
		}
		if(ready > 0){
			wakeUpClient(readFds);
		}

	}

}