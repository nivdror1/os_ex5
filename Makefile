CPP= g++
CPPFLAGS= -c -g -Wextra -Wvla -Wall -std=c++11 -DNDEBUG
TAR_FILES= Makefile README whatsappClient.cpp whatsappServer.cpp

#Exec Files
whatsappClient: whatsappClient.o
	$(CPP) whatsappClient.o -o whatsappClient
	
whatsappServer: whatsappServer.o
	$(CPP) whatsappServer.o -o whatsappServer


# Object Files
whatsappClient.o: whatsappClient.cpp
	$(CPP) $(CPPFLAGS) whatsappClient.cpp -o whatsappClient.o
	
whatsappServer.o: whatsappServer.cpp
	$(CPP) $(CPPFLAGS) whatsappServer.cpp -o whatsappServer.o

tar:
	tar cvf ex5.tar $(TAR_FILES)
 
# Other Targets
clean:
	-rm -f *.o whatsappServer whatsappClient
