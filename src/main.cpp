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

void handleNewConnection(int clientSocket){
	uint8_t playerIdBuf[131];
	ssize_t bytesReceived = recv(clientSocket, playerIdBuf, sizeof(playerIdBuf), 0);

	if(bytesReceived < 130){
		cout << "Receiving player id failed: excepted 130 bytes but got " << bytesReceived << ". Rejecting new connection.\n";
		close(clientSocket);
		return;
	}

	if(playerIdBuf[0] != 0x00){
		cout << "Receiving player id failed: expected packet id to be 0x00 but got 0x" << hex << (int)playerIdBuf[0] << dec << ". Rejecting new connection." << endl;
		close(clientSocket);
		return;
	}

	if(playerIdBuf[1] != 0x07){
		cout << "Receiving player id failed: expected protocol version to be 0x07 but got 0x" << hex << (int)playerIdBuf[1] << dec << ". Rejecting new connection." << endl;
		close(clientSocket);
		return;
	}

	string username = string((char*)&playerIdBuf[2], 64);
	size_t end = username.find_last_not_of(" ");
	if(end != string::npos){
		username = username.substr(0, end + 1);
	}

	cout << "New connection fully identified. Welcome " << username << " to the server!\n";

	sendServerId(clientSocket, "A Server", "Welcome!");
	sendLevelInit(clientSocket);
	sendLevelData(clientSocket);
	sendLevelFinalize(clientSocket);
	
	uint8_t playerId = g_playerManager->addPlayer(clientSocket, username);

	Player* newPlayer = g_playerManager->getPlayer(playerId);
	if(!newPlayer){
		cout << "Error: failed to fetch player from threader. Closing new connection.\n";
		close(clientSocket);
		return;
	}

	newPlayer->x = 2048;
	newPlayer->y = 2112;
	newPlayer->z = 2048;
	newPlayer->yaw = 0;
	newPlayer->pitch = 0;

	{
		lock_guard<mutex> lock(g_playerManager->playersMutex);
		for(auto existingPlayer : g_playerManager->players){
			if(existingPlayer->playerId != playerId && existingPlayer->active){
				sendSpawnPlayer(playerId, existingPlayer->playerId, existingPlayer->username, existingPlayer->x, existingPlayer->y, existingPlayer->z, existingPlayer->yaw, existingPlayer->pitch);
			}
		}
	}

	{
		lock_guard<mutex> lock(g_playerManager->playersMutex);
		for(auto existingPlayer : g_playerManager->players){
			if(existingPlayer->playerId != playerId && existingPlayer->active){
				sendSpawnPlayer(existingPlayer->playerId, playerId, username, newPlayer->x, newPlayer->y, newPlayer->z, newPlayer->yaw, newPlayer->pitch);
			}
		}
	}

	newPlayer->playerThread = thread(playerGameLoop, clientSocket, playerId);
	newPlayer->playerThread.detach();
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

		thread(handleNewConnection, clientSocket).detach();
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
