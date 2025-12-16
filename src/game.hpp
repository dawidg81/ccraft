#ifndef GAME_HPP
#define GAME_HPP

#include <string>

void relayBlock(int clientSocket, short x, short y, short z, uint8_t mode, uint8_t block_id);
void relayPosOrt(int clientSocket, int8_t player_id, short float x, short float y, short float z, uint8_t yaw, uint8_t pitch);
void relayMessage(int clientSocket, int8_t player_id, std::string message);

#endif
