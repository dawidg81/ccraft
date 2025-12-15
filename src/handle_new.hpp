#ifndef HANDLE_FIRST_HPP
#define HANDLE_FIRST_HPP

#include <string>

int recvPlayerId(int &clientSocket);
int sendServerId(int &clientSocket, std::string name, std::string motd);
void sendLevelInit(int &clientSocket);
void sendLevelData(int &clientSocket);
void sendLevelFinalize(int &clientSocket);

#endif
