#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <zlib.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "heartbeat.hpp"
#include "handle_new.hpp"
#include "player_manager.hpp"
#include "game.hpp"

using namespace std;

atomic<bool> running(true);

void heartbeatThread(){
	while(running){
		sendHeartbeat("A Server", 25565, 0, 1, "gregorywashere", true, "ccraft", true);
		this_thread::sleep_for(chrono::seconds(60));
	}
}

void connectionThread(int serverSocket){
	while(running){
		int clientSocket = accept(serverSocket, nullptr, nullptr);
        
		if(clientSocket < 0){
			if(running){
				cerr << "Error accepting connection" << endl;
			}
			continue;
		}
        
		cout << "New connection accepted" << endl;
        
		recvPlayerId(clientSocket);
		sendServerId(clientSocket, "A Server", "Welcome!");
		sendLevelInit(clientSocket);
		sendLevelData(clientSocket);
		sendLevelFinalize(clientSocket);
	}
}

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

	cout << "Server listening on port 25565\n";

	thread hbThread(heartbeatThread);
	thread connThread(connectionThread, serverSocket);

	hbThread.join();
	connThread.join();

	return 0;
}
