#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#include "game.hpp"
#include "player_manager.hpp"

using namespace std;

void sendBlock(uint8_t senderPlayerId, int16_t x, int16_t y, int16_t z, uint8_t block_id) {
    uint8_t buffer[8];
    buffer[0] = 0x06;
    buffer[1] = (x >> 8) & 0xFF;
    buffer[2] = x & 0xFF;
    buffer[3] = (y >> 8) & 0xFF;
    buffer[4] = y & 0xFF;
    buffer[5] = (z >> 8) & 0xFF;
    buffer[6] = z & 0xFF;
    buffer[7] = block_id;
    
    g_playerManager->broadcastToOthers(senderPlayerId, buffer, sizeof(buffer));
}

void sendAbsolutePosOrt(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch) {
    uint8_t buffer[10];
    buffer[0] = 0x08;
    buffer[1] = playerId;
    buffer[2] = (x >> 8) & 0xFF;
    buffer[3] = x & 0xFF;
    buffer[4] = (y >> 8) & 0xFF;
    buffer[5] = y & 0xFF;
    buffer[6] = (z >> 8) & 0xFF;
    buffer[7] = z & 0xFF;
    buffer[8] = yaw;
    buffer[9] = pitch;
    
    g_playerManager->broadcastToOthers(playerId, buffer, sizeof(buffer));
}

void sendRelativePosOrt(uint8_t playerId, int8_t x, int8_t y, int8_t z, uint8_t yaw, uint8_t pitch) {
    uint8_t buffer[7];
    buffer[0] = 0x09;
    buffer[1] = playerId;
    buffer[2] = x;
    buffer[3] = y;
    buffer[4] = z;
    buffer[5] = yaw;
    buffer[6] = pitch;
    
    g_playerManager->broadcastToOthers(playerId, buffer, sizeof(buffer));
}

void sendMessage(uint8_t senderPlayerId, string message) {
    uint8_t buffer[66];
    buffer[0] = 0x0d;
    buffer[1] = senderPlayerId;
    
    memset(&buffer[2], 0x20, 64);
    
    size_t len = message.length();
    if(len > 64) len = 64;
    memcpy(&buffer[2], message.c_str(), len);
    
    g_playerManager->broadcastToAll(buffer, sizeof(buffer));
}

void sendSpawnPlayer(uint8_t targetPlayerId, uint8_t newPlayerId, string username, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch) {
    uint8_t buffer[74];
    buffer[0] = 0x07;
    buffer[1] = newPlayerId;
    
    memset(&buffer[2], 0x20, 64);
    size_t len = username.length();
    if(len > 64) len = 64;
    memcpy(&buffer[2], username.c_str(), len);
    
    buffer[66] = (x >> 8) & 0xFF;
    buffer[67] = x & 0xFF;
    buffer[68] = (y >> 8) & 0xFF;
    buffer[69] = y & 0xFF;
    buffer[70] = (z >> 8) & 0xFF;
    buffer[71] = z & 0xFF;
    buffer[72] = yaw;
    buffer[73] = pitch;
    
    g_playerManager->sendToPlayer(targetPlayerId, buffer, sizeof(buffer));
    g_playerManager->broadcastToOthers(targetPlayerId, buffer, sizeof(buffer));
}

void sendDespawnPlayer(uint8_t playerId) {
    uint8_t buffer[2];
    buffer[0] = 0x0c;
    buffer[1] = playerId;
    
    g_playerManager->broadcastToOthers(playerId, buffer, sizeof(buffer));
}
