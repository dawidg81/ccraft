#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <cstdint>

void sendBlock(uint8_t senderPlayerId, int16_t x, int16_t y, int16_t z, uint8_t block_id);
void sendAbsolutePosOrt(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);
void sendRelativePosOrt(uint8_t playerId, int8_t x, int8_t y, int8_t z, uint8_t yaw, uint8_t pitch);
void sendMessage(uint8_t senderPlayerId, std::string message);
void sendSpawnPlayer(uint8_t targetPlayerId, uint8_t newPlayerId, std::string username, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);
void sendDespawnPlayer(uint8_t playerId);

void recvBlock(uint8_t senderPlayerId, int16_t x, int16_t y, int16_t z, uint8_t block_id);
void recvPosOrt(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);
void recvMessage(uint8_t senderPlayerId, std::string message);
#endif
