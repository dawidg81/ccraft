#ifndef GAME_HPP
#define GAME_HPP

#include <string>

int receiveGameBuffer(int clientSocket);

void sendBlock(int clientSocket, short x, short y, short z, uint8_t block_id);
void sendAbsolutePosOrt(int clientSocket, uint8_t player_id, uint8_t x[2], uint8_t y[2], uint8_t z[2], uint8_t yaw, uint8_t pitch);
void sendRelativePosOrt(int clientSocket, uint8_t player_id, uint8_t x, uint8_t y, uint8_t z, uint8_t yaw, uint8_t pitch);
void sendMessage(int clientSocket, int8_t player_id, std::string message);

#endif
