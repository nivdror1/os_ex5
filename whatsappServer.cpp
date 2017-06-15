#include <stdlib.h>

#include <netdb.h>
#include <map>
#include <vector>
#include <cstring>
#include <zconf.h>
#include <iostream>

#define MAX_CLIENTS 10
#define WELCOME_SOCKET "$welcomeSocket"
#define MAX_CHAR 256
#define MAX_CHAR_CLIENT_NAME 30

/** a map from a client name to a sockId. */
std::map<std::string, int> sockIdentifier;

/** a map from a group name to a vector of client names in matched group. */
std::map<std::string , std::vector<std::string>> groupMembers;

/** a map from client name to a vector of groups that the client is a part of them.*/
std::map<std::string , std::vector<std::string>> clientsToGroups;

fd_set listeningFds;

struct sockaddr_in sa;


/**
 * init
 */
void init(int welcomeSocket, char* port){
	//initiate the fd set listeningFd
	FD_ZERO(&listeningFds);
	FD_SET(welcomeSocket,&listeningFds);
	FD_SET(STDIN_FILENO,&listeningFds);

	//adding the welcomeSocket to the map of sockIdentifier
	sockIdentifier[WELCOME_SOCKET] = welcomeSocket;

	//init the struct sockaddr_in
	memset(&sa, 0, sizeof(struct sockaddr));//todo check if memset can fail
	sa.sin_family= AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	unsigned short portnum = (unsigned short)atoi(port);
	sa.sin_port= htons(portnum);
}

/**
 * clear all the global containers
 */
int finalizer(){
	FD_ZERO(&listeningFds);
	sockIdentifier.clear();
	groupMembers.clear();
	clientsToGroups.clear();
	//todo delete sockaddr_in sa
	return 0;
}

/**
 * send a message to the client
 * @param socketFd the socket file descriptor
 * @param message the message to be sent
 */
void sendMessageToClient(int socketFd, std::string message){
	if(send(socketFd,message.c_str(),message.size(),0) != (ssize_t)message.size()){
		//todo error
	}
}

/**
 * search the map for the client name, if case the client name is not in the map
 * add it and send "connected' message , else send "the client name already exists"
 * @param newSocket the new socket just connected to the server
 * @param name the client name
 */
void sendConnectionMessage(int newSocket, void* name){
	std::string message;
	//search the map for the client name
	char* clientName = (char*) name;
	auto searchClientName = sockIdentifier.find(clientName);

	//in case the client name isn't in the map , add it
	if(searchClientName == sockIdentifier.end()){
		sockIdentifier[clientName] = newSocket;
		//output server
		std::cout<< clientName<<" connected."<<std::endl;
		//send a message to the client
		message = "Connected successfully";

	}else{
		message = "Connected Failed";
	}
	sendMessageToClient(newSocket, message);
}

/**
 * connceting a new client , and send a message to client whether the connection was a success or not
 * and add him to the map of socketIdentifier
 */
void connectNewClient(){
	void *clientName[MAX_CHAR_CLIENT_NAME];
	//accept the new client
	int addressLength = sizeof(sa);
	int newSocket = accept(sockIdentifier.at(WELCOME_SOCKET),(struct sockaddr *)&sa, (socklen_t*)&addressLength );
	if(newSocket < 0){
		//todo error
	}
	//read the message
	if(read(newSocket,clientName,MAX_CHAR_CLIENT_NAME)< 0){
		//todo error
	}
	//send a message whether the connection has succeed
	sendConnectionMessage( newSocket,clientName);
}

/**
 * exit the server when the server has recieved an exit command
 */
void exitServer(){
	finalizer();
	//todo maybe close all of the fd
	std::cout<<"EXIT command is typed: server is shutting down"<<std::endl;
	exit(0);
}

/**
 * send all of the connected client at the "who" request
 * @param clientName the client name requested it
 */
void sendConnectedClients(std::string clientName){
	//todo you haven't sorted the client list, and how does a list of all of the user connected
	//todo can be empty? do we need to return list without the client requesting it?
	std::string sortedClients;
	for (auto iter = sockIdentifier.begin(); iter != sockIdentifier.end(); ++iter)
	{
		sortedClients.append(iter->first + ",");
	}
	sortedClients.append("\n");
	sendMessageToClient(sockIdentifier.at(clientName), sortedClients.substr(0, sortedClients.size()-1)); // erase last comma

}

/**
 * remove a client from the data structures
 * @param clientName
 */
void removeClient(std::string clientName){
	//get all of the groups the said client name appear in them
	auto groupsNames = clientsToGroups.at(clientName);
	//for each group erase the client name from group member map
	for(unsigned int i = 0;i<groupsNames.size(); i++) {
		auto curIter = groupMembers.find(groupsNames.at(i));
		std::vector curGroup = (*curIter).second;
		curGroup.erase( std::find(curGroup.begin(),curGroup.end(),clientName));
	}
	//erase from the clientToGroups map
	clientsToGroups.erase(clientName);
	//todo send a message to the client

}

/**
 * parse and execute the commands from the client
 * @param clientName the current client name
 * @param message the message recieved from the client
 */
void parseAndExec(std::string clientName, std::string message){
	auto commandEndIndex = message.find_first_of(" ");
	std::string command = message.substr(0, commandEndIndex);
	std::string restOfMessage = message.substr(commandEndIndex, std::string::npos);
	if (command == "create_group"){
		createNewGroup(clientName, restOfMessage);
	} else if (command == "send"){
		sendMessage(clientName, restOfMessage);
	} else if (command == "who"){
		sendConnectedClients(clientName);
	} else {
		removeClient(clientName);
	}
}

/**
 * for each fd check if a fd is the set ,is so read from it
 * @param readFds the fd set that is ready to be read
 * @param ready the readFds set size
 */
void handleClientRequest(int ready,fd_set &readFds) {
	void buf[MAX_CHAR];

	for(auto iter = sockIdentifier.begin(); iter!= sockIdentifier.end();++iter){
		//check if the fd is in the set and read from it
		if (FD_ISSET((*iter).second,&readFds)){
			memset(buf, '0', sizeof(buf));
			if(read((*iter).second, buf, MAX_CHAR)< 0){
				//todo error
			}
			//parse the messages and executes them
			parseAndExec((*iter).first, (char*)buf);
			ready--;
		}
		if(ready == 0){
			break;
		}
	}
}


/**
 * check which fd is ready to be read, if it is the welcomeSocket we'll connect a new client
 * if it's the std_in we'll exit the server else recieve comunication from the client.
 * @param readFds the fd set that is ready to be read
 * @param ready the readFds set size
 */
void wakeUpServer(fd_set &readFds, int ready){
	//connect the new client
	if(FD_ISSET(sockIdentifier.at(WELCOME_SOCKET),&readFds)){
		connectNewClient();
		FD_CLR(sockIdentifier.at(WELCOME_SOCKET), &readFds);
		ready--;
	}
	//exit the server
	if(FD_ISSET(STDIN_FILENO,&readFds)){
		exitServer();
	}else{
		//receive and send messages from/to the client
		handleClientRequest(ready, readFds);
	}
}

int main(int argc, char *argv[]){

	if(argc != 2){
		//todo error
		return -1;
	}
	//create a new socket
	int welcomeSocket = socket(AF_INET,SOCK_STREAM,0);
	if(welcomeSocket  == -1){
		//todo error
		return -1;
	}

	init(welcomeSocket,argv[1]);
	//bind the main server socket
	if (bind(welcomeSocket , (struct sockaddr *)&sa , sizeof(sa)) < 0) {
		close(welcomeSocket );//todo error close can fail
		return(-1);
	}

	if(listen(welcomeSocket,MAX_CLIENTS) == -1){
		//todo error
		return -1;
	}

	bool stillRunning = true;
	while(stillRunning){
		fd_set readFds = listeningFds;
		int ready;
		if((ready = select(MAX_CLIENTS+1, &readFds, NULL, NULL,NULL))<0){
			//todo error
			return -1;
		}
		if(ready > 0){
			wakeUpServer(readFds,ready);
		}

	}
	return 0;

}


