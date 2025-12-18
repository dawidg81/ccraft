#include <iostream>
#include <algorithm>
#include <unistd.h>
#include <cstring>
#include <netinet/in.h>

#include "player_manager.hpp"
#include "game.hpp"

PlayerManager* g_playerManager = nullptr;

PlayerManager::PlayerManager() : nextPlayerId(1) {
    std::cout << "PlayerManager initialized\n";
}

PlayerManager::~PlayerManager() {
    std::lock_guard<std::mutex> lock(playersMutex);
    for(auto player : players) {
        player->active = false;
        close(player->clientSocket);
        if(player->playerThread.joinable()) {
            player->playerThread.join();
        }
        delete player;
    }
    players.clear();
}

uint8_t PlayerManager::addPlayer(int clientSocket, std::string username) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    uint8_t playerId = nextPlayerId++;
    Player* newPlayer = new Player(clientSocket, playerId, username);
    players.push_back(newPlayer);
    
    std::cout << "Player " << username << " added with ID " << (int)playerId << "\n";
    
    return playerId;
}

void PlayerManager::removePlayer(uint8_t playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    auto it = std::find_if(players.begin(), players.end(), 
        [playerId](Player* p) { return p->playerId == playerId; });
    
    if(it != players.end()) {
        Player* player = *it;
        player->active = false;
        
        std::cout << "Removing player " << player->username << " (ID: " << (int)playerId << ")\n";
        
        close(player->clientSocket);
        players.erase(it);
        delete player;
    }
}

void PlayerManager::broadcastToOthers(uint8_t senderPlayerId, const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    for(auto player : players) {
        if(player->playerId != senderPlayerId && player->active) {
            send(player->clientSocket, data, length, 0);
        }
    }
}

void PlayerManager::broadcastToAll(const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    for(auto player : players) {
        if(player->active) {
            send(player->clientSocket, data, length, 0);
        }
    }
}

void PlayerManager::sendToPlayer(uint8_t playerId, const uint8_t* data, size_t length) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    auto it = std::find_if(players.begin(), players.end(), 
        [playerId](Player* p) { return p->playerId == playerId; });
    
    if(it != players.end() && (*it)->active) {
        send((*it)->clientSocket, data, length, 0);
    }
}

Player* PlayerManager::getPlayer(uint8_t playerId) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    auto it = std::find_if(players.begin(), players.end(), 
        [playerId](Player* p) { return p->playerId == playerId; });
    
    if(it != players.end()) {
        return *it;
    }
    return nullptr;
}

void PlayerManager::updatePlayerPosition(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch) {
    std::lock_guard<std::mutex> lock(playersMutex);
    
    auto it = std::find_if(players.begin(), players.end(), 
        [playerId](Player* p) { return p->playerId == playerId; });
    
    if(it != players.end()) {
        (*it)->x = x;
        (*it)->y = y;
        (*it)->z = z;
        (*it)->yaw = yaw;
        (*it)->pitch = pitch;
    }
}

void playerGameLoop(int clientSocket, uint8_t playerId) {
    std::cout << "Game loop started for player ID " << (int)playerId << "\n";
    
    Player* player = g_playerManager->getPlayer(playerId);
    if(!player) {
        close(clientSocket);
        return;
    }
    
    while(player->active) {
        uint8_t packetId;
        ssize_t bytesReceived = recv(clientSocket, &packetId, 1, MSG_PEEK);
        
        if(bytesReceived <= 0) {
            std::cout << "Player " << (int)playerId << " disconnected\n";
            break;
        }
        
        if(packetId == 0x05) {
            uint8_t buffer[9];
            bytesReceived = recv(clientSocket, buffer, 9, 0);
            if(bytesReceived != 9) break;
            
            int16_t x = (static_cast<int16_t>(buffer[1]) << 8) | buffer[2];
            int16_t y = (static_cast<int16_t>(buffer[3]) << 8) | buffer[4];
            int16_t z = (static_cast<int16_t>(buffer[5]) << 8) | buffer[6];
            uint8_t mode = buffer[7];
            uint8_t block_id = buffer[8];
            
            std::cout << "Player " << (int)playerId << " set block at (" << x << "," << y << "," << z << ") to " << (int)block_id << "\n";
            
            sendBlock(playerId, x, y, z, block_id);
            
        } else if(packetId == 0x08) {
            uint8_t buffer[10];
            bytesReceived = recv(clientSocket, buffer, 10, 0);
            if(bytesReceived != 10) break;
            
            int16_t x = (static_cast<int16_t>(buffer[2]) << 8) | buffer[3];
            int16_t y = (static_cast<int16_t>(buffer[4]) << 8) | buffer[5];
            int16_t z = (static_cast<int16_t>(buffer[6]) << 8) | buffer[7];
            uint8_t yaw = buffer[8];
            uint8_t pitch = buffer[9];
            
            g_playerManager->updatePlayerPosition(playerId, x, y, z, yaw, pitch);
            sendAbsolutePosOrt(playerId, x, y, z, yaw, pitch);
            
        } else if(packetId == 0x0d) {
            uint8_t buffer[66];
            bytesReceived = recv(clientSocket, buffer, 66, 0);
            if(bytesReceived != 66) break;
            
            char message[65];
            memcpy(message, &buffer[2], 64);
            message[64] = '\0';
            
            for(int i = 63; i >= 0; i--) {
                if(message[i] == ' ' || message[i] == '\0') {
                    message[i] = '\0';
                } else {
                    break;
                }
            }
            
            std::cout << "Player " << (int)playerId << " says: " << message << "\n";
            sendMessage(playerId, std::string(message));
            
        } else {
            std::cout << "Player " << (int)playerId << " sent invalid packet 0x" << std::hex << (int)packetId << std::dec << "\n";
            break;
        }
    }
    
    sendDespawnPlayer(playerId);
    g_playerManager->removePlayer(playerId);
}
