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

static inline int16_t readInt16BE(const uint8_t* b) {
    return (int16_t)((b[0] << 8) | b[1]);
}

void playerGameLoop(int clientSocket, uint8_t playerId)
{
    std::cout << "Game loop started for player ID " << (int)playerId << "\n";

    while (true)
    {
        uint8_t packetId;
        ssize_t r = recv(clientSocket, &packetId, 1, MSG_WAITALL);
        if (r <= 0) break;

        switch (packetId)
        {
            // ─────────────
            // 0x05 Set Block
            // ─────────────
            case 0x05:
            {
                uint8_t buf[8];
                if (recv(clientSocket, buf, 8, MSG_WAITALL) <= 0)
                    goto disconnect;

                int16_t x = readInt16BE(&buf[0]);
                int16_t y = readInt16BE(&buf[2]);
                int16_t z = readInt16BE(&buf[4]);
                uint8_t mode = buf[6];      // optional
                uint8_t block = buf[7];

                recvBlock(playerId, x, y, z, block);
                break;
            }

            // ─────────────────────────
            // 0x08 Position & Orientation
            // ─────────────────────────
            case 0x08:
            {
                uint8_t buf[9];
                if (recv(clientSocket, buf, 9, MSG_WAITALL) <= 0)
                    goto disconnect;

                // buf[0] = client playerId (always 255)
                int16_t x = readInt16BE(&buf[1]);
                int16_t y = readInt16BE(&buf[3]);
                int16_t z = readInt16BE(&buf[5]);
                uint8_t yaw   = buf[7];
                uint8_t pitch = buf[8];

                recvPosOrt(playerId, x, y, z, yaw, pitch);
                break;
            }

            // ─────────────
            // 0x0D Chat
            // ─────────────
            case 0x0D:
            {
                uint8_t buf[65];
                if (recv(clientSocket, buf, 65, MSG_WAITALL) <= 0)
                    goto disconnect;

                std::string msg((char*)&buf[1], 64);
                size_t end = msg.find_last_not_of(' ');
                if (end != std::string::npos)
                    msg.resize(end + 1);
                else
                    msg.clear();

                recvMessage(playerId, msg);
                break;
            }

            default:
                std::cout << "Unknown packet 0x"
                          << std::hex << (int)packetId << std::dec << "\n";
                goto disconnect;
        }
    }

disconnect:
    std::cout << "Player " << (int)playerId << " disconnected\n";
    sendDespawnPlayer(playerId);
    g_playerManager->removePlayer(playerId);
    close(clientSocket);
}

