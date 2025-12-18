#ifndef GAME_HPP
#define GAME_HPP

#include <string>
#include <cstdint>

// Send block change to other players
void sendBlock(uint8_t senderPlayerId, int16_t x, int16_t y, int16_t z, uint8_t block_id);

// Send absolute position and orientation
void sendAbsolutePosOrt(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);

// Send relative position and orientation update
void sendRelativePosOrt(uint8_t playerId, int8_t x, int8_t y, int8_t z, uint8_t yaw, uint8_t pitch);

// Send chat message
void sendMessage(uint8_t senderPlayerId, std::string message);

// Spawn a player for another player (when new player joins, tell others about them)
void sendSpawnPlayer(uint8_t targetPlayerId, uint8_t newPlayerId, std::string username, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);

// Despawn a player (when player disconnects)
void sendDespawnPlayer(uint8_t playerId);

#endif
