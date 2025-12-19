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


void handleNewConnection(int clientSocket) {
    // Step 1: Receive player identification
    uint8_t playerIdBuf[131];
    ssize_t bytesReceived = recv(clientSocket, playerIdBuf, sizeof(playerIdBuf), 0);

    if (bytesReceived < 130) {
        cout << "Failed to receive player identification (got " << bytesReceived << " bytes)\n";
        close(clientSocket);
        return;
    }

    // Validate packet ID
    if (playerIdBuf[0] != 0x00) {
        cout << "Invalid packet ID: expected 0x00 but got 0x" << hex << (int)playerIdBuf[0] << dec << "\n";
        close(clientSocket);
        return;
    }

    // Validate protocol version
    if (playerIdBuf[1] != 0x07) {
        cout << "Invalid protocol version: expected 0x07 but got 0x" << hex << (int)playerIdBuf[1] << dec << "\n";
        close(clientSocket);
        return;
    }

    // Extract and trim username
    string username((char*)&playerIdBuf[2], 64);
    size_t end = username.find_last_not_of(" ");
    if (end != string::npos) username = username.substr(0, end + 1);

    cout << "New connection fully identified. Welcome " << username << " to the server!\n";

    // Step 2: Send initial server/level packets
    sendServerId(clientSocket, "A Server", "Welcome!");
    sendLevelInit(clientSocket);
    sendLevelData(clientSocket);
    sendLevelFinalize(clientSocket);

    // Step 3: Add player to manager
    uint8_t playerId = g_playerManager->addPlayer(clientSocket, username);

    // Step 4: Get new player object
    Player* newPlayer = g_playerManager->getPlayer(playerId);
    if (!newPlayer) {
        cout << "Error: Failed to get player after adding\n";
        close(clientSocket);
        return;
    }

    // Set initial spawn position
    newPlayer->x = 2048;
    newPlayer->y = 2112;
    newPlayer->z = 2048;
    newPlayer->yaw = 0;
    newPlayer->pitch = 0;

    // Step 5: Send initial teleport to self (0x08)
    {
        uint8_t buffer[10];
        buffer[0] = 0x08;
        buffer[1] = playerId;
        buffer[2] = (newPlayer->x >> 8) & 0xFF;
        buffer[3] = newPlayer->x & 0xFF;
        buffer[4] = (newPlayer->y >> 8) & 0xFF;
        buffer[5] = newPlayer->y & 0xFF;
        buffer[6] = (newPlayer->z >> 8) & 0xFF;
        buffer[7] = newPlayer->z & 0xFF;
        buffer[8] = newPlayer->yaw;
        buffer[9] = newPlayer->pitch;

        g_playerManager->sendToPlayer(playerId, buffer, sizeof(buffer));
    }

    // Step 6: Spawn existing players for the new player
    {
        lock_guard<mutex> lock(g_playerManager->playersMutex);
        for (auto existingPlayer : g_playerManager->players) {
            if (existingPlayer->playerId != playerId && existingPlayer->active) {
                // Send spawn packet (0x07) to new player
                sendSpawnPlayer(playerId, existingPlayer->playerId,
                                existingPlayer->username,
                                existingPlayer->x, existingPlayer->y, existingPlayer->z,
                                existingPlayer->yaw, existingPlayer->pitch);

                // Send exact position/rotation (0x08) to new player
                uint8_t buffer[10];
                buffer[0] = 0x08;
                buffer[1] = existingPlayer->playerId;
                buffer[2] = (existingPlayer->x >> 8) & 0xFF;
                buffer[3] = existingPlayer->x & 0xFF;
                buffer[4] = (existingPlayer->y >> 8) & 0xFF;
                buffer[5] = existingPlayer->y & 0xFF;
                buffer[6] = (existingPlayer->z >> 8) & 0xFF;
                buffer[7] = existingPlayer->z & 0xFF;
                buffer[8] = existingPlayer->yaw;
                buffer[9] = existingPlayer->pitch;

                g_playerManager->sendToPlayer(playerId, buffer, sizeof(buffer));
            }
        }
    }

    // Step 7: Spawn new player for all existing players
    {
        lock_guard<mutex> lock(g_playerManager->playersMutex);
        for (auto existingPlayer : g_playerManager->players) {
            if (existingPlayer->playerId != playerId && existingPlayer->active) {
                // Send spawn packet (0x07) to existing player
                sendSpawnPlayer(existingPlayer->playerId, playerId,
                                newPlayer->username,
                                newPlayer->x, newPlayer->y, newPlayer->z,
                                newPlayer->yaw, newPlayer->pitch);

                // Send initial absolute position (0x08) of new player to others
                uint8_t buffer[10];
                buffer[0] = 0x08;
                buffer[1] = playerId;
                buffer[2] = (newPlayer->x >> 8) & 0xFF;
                buffer[3] = newPlayer->x & 0xFF;
                buffer[4] = (newPlayer->y >> 8) & 0xFF;
                buffer[5] = newPlayer->y & 0xFF;
                buffer[6] = (newPlayer->z >> 8) & 0xFF;
                buffer[7] = newPlayer->z & 0xFF;
                buffer[8] = newPlayer->yaw;
                buffer[9] = newPlayer->pitch;

                g_playerManager->sendToPlayer(existingPlayer->playerId, buffer, sizeof(buffer));
            }
        }
    }

    // Step 8: Start game loop in a new thread
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
        
        // Handle each connection in a separate thread so we don't block accepting more
        thread(handleNewConnection, clientSocket).detach();
    }
}

int main(){
    cout << "ccraft v0.0.0" << endl;
    
    // CRITICAL: Initialize player manager FIRST before any threads
    g_playerManager = new PlayerManager();
    
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    // Set SO_REUSEADDR to avoid "Address already in use" errors
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(25565);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    if(::bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) != 0){
        cout << "Fatal error: failed to bind to the address. Exiting with code 1.\n";
        delete g_playerManager;
        return 1;
    }
    
    if(listen(serverSocket, 5) != 0){
        cout << "Fatal error: failed to listen to port. Exiting with code 2.\n";
        delete g_playerManager;
        return 2;
    }
    
    cout << "Server listening on port 25565\n";
    
    thread hbThread(heartbeatThread);
    thread connThread(connectionThread, serverSocket);
    
    hbThread.join();
    connThread.join();
    
    // Cleanup
    delete g_playerManager;
    close(serverSocket);
    
    return 0;
}
