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

#define MAX_CLIENTS_CONNECTED 30
#define MAX_CLIENTS 10
#define WELCOME_SOCKET "$welcomeSocket"
#define MAX_CHAR 256
#define MAX_CHAR_CLIENT_NAME 30

/** a map from a client name to a sockId. */
std::map<std::string, int> sockIdentifier;

/** a map from a group name to a vector of client names in matched group. */
std::map<std::string , std::set<std::string>> groupNameToClients;

/** a map from client name to a vector of groups that the client is a part of them.*/
std::map<std::string , std::vector<std::string>> clientsToGroups;

fd_set listeningFds;

struct sockaddr_in sa;

typedef std::map<std::string , std::vector<std::string>>::iterator clientsToGroupsIter;


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
	memset(&sa, 0, sizeof(struct sockaddr));
	sa.sin_family= AF_INET;
	sa.sin_addr.s_addr = htonl(INADDR_ANY);
	unsigned short portnum = (unsigned short)atoi(port);
	sa.sin_port= htons(portnum);
}


/**
 * send a message to the client
 * @param socketFd the socket file descriptor
 * @param message the message to be sent
 */
void sendMessageToClient(int socketFd, std::string message){
	if(send(socketFd,message.c_str(),message.size(),0) != (ssize_t)message.size()){
        std::cerr << "ERROR: send " << errno << ".\n";
        exit(1);
	}
}

std::string addClientToDatastructures(int newSocket, char* clientName){
    sockIdentifier[clientName] = newSocket;
    std::vector<std::string> groupName;
    clientsToGroups[clientName]= groupName;
    //output server
    std::cout<< clientName << " connected."<<std::endl;
    //send a message to the client
    return "Connected successfully.\n";
}

/**
 * search the map for the client name, if case the client name is not in the map
 * add it and send "connected' message , else send "the client name already exists"
 * @param newSocket the new socket just connected to the server
 * @param name the client name
 */
void sendConnectionMessage(int newSocket, void* name){
	//search the map for the client name
	char* clientName = (char*) name;
	auto searchClientName = sockIdentifier.find(clientName);

	//in case the client name isn't in the map , add it
	if(searchClientName == sockIdentifier.end()){
        sendMessageToClient(newSocket, addClientToDatastructures(newSocket, clientName));
	}else{
        sendMessageToClient(newSocket, "Client name is already in use.\n");
	}
}

/**
 * connceting a new client , and send a message to client whether the connection was a success or not
 * and add him to the map of socketIdentifier
 */
void connectNewClient(){
	void *clientName[MAX_CHAR_CLIENT_NAME];
	int addressLength = sizeof(sa);
	//check for the max clients connected
	if(sockIdentifier.size() == 31){
        return; //todo error check in the future
	}
    //accept the new client
	int newSocket = accept(sockIdentifier.at(WELCOME_SOCKET),(struct sockaddr *)&sa, (socklen_t*)&addressLength );
	if(newSocket < 0){
        std::cerr << "ERROR: accept " << errno << ".\n";
        exit(1);
	}
	//read the message
	if(read(newSocket,clientName,MAX_CHAR_CLIENT_NAME)< 0){
		//send to the client that he didn't connect to the client
		sendMessageToClient( newSocket,"Failed to connect the server.\n");
        std::cerr << "ERROR: read " << errno << ".\n";
        exit(1);
	}
	//send a message whether the connection has succeed
	sendConnectionMessage( newSocket,clientName);
}

/**
 * exit the server when the server has recieved an exit command
 */
void exitServer(){
	char buf[MAX_CHAR];
    ssize_t numberOfBytesRead = read(STDIN_FILENO, buf, MAX_CHAR);
	if (numberOfBytesRead < 0){
        std::cerr << "ERROR: read " << errno << ".\n";
        exit(1);
	}
    std::string bufferString(buf);

	if (bufferString.substr(0, numberOfBytesRead) == "EXIT"){
        std::cerr << "ERROR: Invalid input.\n";
        exit(1);
	}

	FD_ZERO(&listeningFds);
	//close all socket fd's
	for (auto iter = sockIdentifier.begin(); iter!= sockIdentifier.end(); ++iter){
        if ((*iter).first != WELCOME_SOCKET){
            sendMessageToClient((*iter).second, "Shut down server.\n");
        }
		if(close((*iter).second)==-1){
            std::cerr << "ERROR: close " << errno << ".\n";
            exit(1);
		}
	}
	//clear the maps
	sockIdentifier.clear();
	groupNameToClients.clear();
	clientsToGroups.clear();

	std::cout<<"EXIT command is typed: server is shutting down.\n";
	exit(0);
}

/**
 * send all of the connected client at the "who" request
 * @param clientName the client name requested it
 */
void sendConnectedClients(std::string clientName){

	//append all client to a string
	std::string sortedClients;
	for (auto iter = sockIdentifier.begin(); iter != sockIdentifier.end(); ++iter)
	{
		// do not append the client name to the string of sorted clients
		if(clientName != (*iter).first &&  WELCOME_SOCKET != (*iter).first) {
			sortedClients.append(iter->first + ",");
		}
	}
	sortedClients.erase(sortedClients.size()-1,1); //remove the final comma
	sortedClients.append("\n");
	//send a message to the client containing the clients
	sendMessageToClient(sockIdentifier.at(clientName), sortedClients);

	//output server
	std::cout<< clientName + ": Requests the currently connected client names.\n";

}

void removeClientFromGroups(clientsToGroupsIter groupsNamesIter, std::string clientName){
    std::vector<std::string> groupNamesVec = (*groupsNamesIter).second;
    //for each group erase the client name from group member map
    for (unsigned int i = 0; i < groupNamesVec.size(); i++) {
        auto curIter = groupNameToClients.find(groupNamesVec.at(i));
        std::set<std::string>& curGroup = (*curIter).second;
        curGroup.erase(std::find(curGroup.begin(), curGroup.end(), clientName));
    }
    //erase from the clientToGroups map
    clientsToGroups.erase(clientName);
}

/**
 * remove a client from the data structures
 * @param clientName
 */
void removeClient(std::string clientName){
	//get all of the groups the said client name appear in them
	auto groupsNamesIter = clientsToGroups.find(clientName);
	if(groupsNamesIter !=  clientsToGroups.end()) {
        removeClientFromGroups(groupsNamesIter, clientName);
	}

	// send a message to the client
	sendMessageToClient(sockIdentifier.at(clientName),"Unregistered successfully.\n");
	//output server
	std::cout<< clientName + ": Unregistered successfully.\n";

	if(close (sockIdentifier.at(clientName))== -1){
        std::cerr << "ERROR: close " << errno << ".\n";
        exit(1);
	}
	sockIdentifier.erase(clientName);

}

/**
 * send an error message to the client and output from the server
 * @param clientName the client name whom send the message
 * @param groupName the group name who failed to create
 */
 void createGroupErrorMessage(std::string  &clientName, std::string &groupName){
	 sendMessageToClient(sockIdentifier.at(clientName), "ERROR: failed to create group \"" + groupName + "\".\n");
	 std::cerr << clientName + ": ERROR: failed to create group \"" + groupName + "\".\n";
 }

/**
 * check if the clients exists and insert them to the set
 * @param clientName the client name whom send the message
 * @param groupName the group name supposed to be created.
 * @param groupNameAndMembers a vector containing the group name and client name
 */
std::set<std::string> checkIfClientsExists(std::string  &clientName,
                                              std::string &groupName, std::vector<std::string> &groupMembers){
	std::set<std::string> clientSet;
	//define iterators
	auto begin = groupMembers.begin();
	auto end = groupMembers.end();
	for( auto iter= begin; iter != end;++iter){

		//check if the client exists if not send an error!!!
		if(sockIdentifier.find(*iter)== sockIdentifier.end()){
			createGroupErrorMessage(clientName, groupName); //output the error message
			clientSet.clear();
			return clientSet;
		}
		//insert to the client set
		clientSet.insert((*iter));

	}
	//add the client name (the sender)
	clientSet.insert(clientName);
	return clientSet;
}

/**
 * create a new group, while the group haven't been created
 * and there are at least two client in it
 * @param clientName the client name whom send the message
 * @param groupNameAndMembers a vector containing the group name and client name
 */
void createNewGroup(std::string &clientName, std::string &groupNameAndMembers){
	auto splitGroupNameAndMembers = split(groupNameAndMembers, ' ', 30);
	//getting the group name
	std::string groupName = splitGroupNameAndMembers.at(0);

	//check if the group name already exists
	if(groupNameToClients.find(groupName) != groupNameToClients.end()){
		createGroupErrorMessage(clientName, groupName); //output the error message
		return;
	}
	//erase the command and group name from the vector
	splitGroupNameAndMembers.erase(splitGroupNameAndMembers.begin());

	//check if the clients exists and insert them to the set
	std::set<std::string> clientSet = checkIfClientsExists(clientName,groupName, splitGroupNameAndMembers);
	if(clientSet.size() == 0){
		return;
	}

	//add to the maps
	groupNameToClients[groupName]= clientSet;

	for(auto iter = clientSet.begin();iter != clientSet.end();++iter){
		clientsToGroups.at((*iter)).push_back(groupName);
	}

	//output a success message
	sendMessageToClient(sockIdentifier.at(clientName), "Group \"" + groupName + "\" was created successfully.\n" );
	std::cout<< clientName + ": Group \"" + groupName+"\" was created successfully.\n";

}

void sendMessageToGroup(std::string &senderName, std::string &receiverName, std::string &text,
                        std::string &message){
    std::set<std::string> groupUsers = groupNameToClients.at(receiverName) ;
    if (groupUsers.find(senderName) == groupUsers.end()){
        sendMessageToClient(sockIdentifier.at(senderName), "ERROR: failed to send.\n");
        std::cerr<< senderName + ": ERROR: failed to send \"" + message + "\" to "+ receiverName+ ""
                                                                                                       ".\n";
        return;
    }
    for (auto iter= groupUsers.begin(); iter != groupUsers.end();++iter){
        if((*iter) != senderName){
            sendMessageToClient(sockIdentifier.at((*iter)),text+ message + "\n");
        }
    }
    sendMessageToClient(sockIdentifier.at(senderName), "Sent successfully.\n");
    std::cout<< text + "\"" + message + "\" was sent successfully to " + receiverName + ".\n";
}

void sendMessage(std::string &senderName, std::string &splitMessage){
	auto receiverAndMessage = split(splitMessage, ' ', 1);
	std::string receiverName = receiverAndMessage.at(0);
	//decipher the message
	std::string text = senderName +": ";
	std::string message = receiverAndMessage.at(1);

	//check if the receiver name is a client then send the message only to him
	if(sockIdentifier.find(receiverName) != sockIdentifier.end()){
		sendMessageToClient(sockIdentifier.at(receiverName),text+ message+"\n");
		sendMessageToClient(sockIdentifier.at(senderName), "Sent successfully.\n");
		std::cout<<text + "\"" + message + "\" was sent successfully to " + receiverName + ".\n";
	}

	//check if the receiver name is a group then send the message only to the group user's
	//without sending it the to sender client
	else if(groupNameToClients.find(receiverName) != groupNameToClients.end()){
        sendMessageToGroup(senderName, receiverName, text, message);
	}else{
		sendMessageToClient(sockIdentifier.at(senderName), "ERROR: failed to send.\n");
		std::cerr<< senderName + ": ERROR: failed to send \"" + message + "\" to "+ receiverName+ ""
                                                                                                       ".\n";
	}


}

/**
 * parse and execute the commands from the client
 * @param clientName the current client name
 * @param message the message recieved from the client
 */
void parseAndExec(std::string clientName, std::string message){
	auto splitMessage = split(message, ' ', 1);


	std::string command = splitMessage.at(0);
	splitMessage.erase(splitMessage.begin());
	if (command == "create_group"){
		createNewGroup(clientName, splitMessage.at(0));
	} else if (command == "send"){
		sendMessage(clientName, splitMessage.at(0));
	} else if (command.substr(0,3) == "who"){
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
	char buf[MAX_CHAR];

	for(auto iter = sockIdentifier.begin(); iter!= sockIdentifier.end();++iter){
		//check if the fd is in the set and read from it
		if (FD_ISSET((*iter).second,&readFds)){
			memset(buf, '0', sizeof(buf));
			ssize_t numberOfBytesRead = read((*iter).second, buf, MAX_CHAR);
			if(numberOfBytesRead < 0){
                std::cerr << "ERROR: read " << errno << ".\n";
                exit(1);
            }
			buf[numberOfBytesRead] = '\0';
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
 * @param welcomeSocket the fd of the listening socket
 */
void wakeUpServer(fd_set &readFds, int ready, int welcomeSocket){
	//connect the new client
	if(FD_ISSET(welcomeSocket,&readFds)){
		connectNewClient();
		FD_CLR(welcomeSocket, &readFds);
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

/**
 * reset the set containing fd's
 */
void resetFdSet(){
    FD_ZERO(&listeningFds);
    for(auto iter=  sockIdentifier.begin();iter!=sockIdentifier.end();++iter){
        FD_SET((*iter).second,&listeningFds);
    }
    FD_SET(STDIN_FILENO,&listeningFds);
}

int main(int argc, char *argv[]){

	if(argc != 2){
        std::cerr << "Usage: whatsappServer portNum" << std::endl;
        exit(1);
	}
	//create a new socket
	int welcomeSocket = socket(AF_INET,SOCK_STREAM,0);
	if(welcomeSocket  == -1){
        std::cerr << "ERROR: socket " << errno << ".\n";
        exit(1);
	}

	init(welcomeSocket,argv[1]);
	//bind the main server socket
	if (bind(welcomeSocket , (struct sockaddr *)&sa , sizeof(sa)) < 0) {
        std::cerr << "ERROR: bind " << errno << ".\n";
        if (close(welcomeSocket) < 0){
            std::cerr << "ERROR: close " << errno << ".\n";
        }
        exit(1);
	}

	if(listen(welcomeSocket,MAX_CLIENTS) == -1)
    {
        std::cerr << "ERROR: listen " << errno << ".\n";
        exit(1);
    }
	while(true){
        resetFdSet();
        fd_set readFds = listeningFds;
		int ready;

		if((ready = select(FD_SETSIZE, &readFds, NULL, NULL,NULL))<0){
            std::cerr << "ERROR: select " << errno << ".\n";
            return -1;
		}
		if(ready > 0){
			wakeUpServer(readFds,ready,welcomeSocket);
		}

	}

}


