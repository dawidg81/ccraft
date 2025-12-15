#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <zlib.h>

#include "heartbeat.hpp"
#include "handle_first.hpp"

using namespace std;

int main(){
	cout << "ccraft v0.0.0" << endl;

	int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(25565);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	if(::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0){
		cout << "Fatal error: failed to bind to the address. Exiting with code 1.\n";
		return 1;
	}

	if(listen(serverSocket, 5) != 0){
		cout << "Fatal error: failed to listen to port. Exiting with code 2.\n";
		return 2;
	}

	sendHeartbeat("ccraft", 25565, 0, 1, "gregorywashere", true);
	
	while(true){
		int clientSocket = accept(serverSocket, nullptr, nullptr);
		recvPlayerId(clientSocket);
		sendServerId(clientSocket, "A Server", "Welcome!");
		sendLevelInit(clientSocket);
		sendLevelData(clientSocket);
		sendLevelFinalize(clientSocket);
	}
	return 0;
}
