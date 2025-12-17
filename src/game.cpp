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
#include "game.hpp"

using namespace std;

int receiveGameBuffer(int clientSocket){
	uint8_t newBuffer[74];

	recv(clientSocket, newBuffer, sizeof(newBuffer), 0);
	uint8_t new_pack_id;
	new_pack_id = newBuffer[0];

	if(new_pack_id == 0x05){
		int8_t x = (static_cast<int16_t>(newBuffer[1]) << 8) | newBuffer[2];
		int8_t y = (static_cast<int16_t>(newBuffer[3]) << 8) | newBuffer[4];
		int8_t z = (static_cast<int16_t>(newBuffer[5]) << 8) | newBuffer[6];
		uint8_t mode = newBuffer[7];
		uint8_t block_id = newBuffer[8];
	} else if(new_pack_id == 0x08){
		uint8_t player_id = 0xFF;
		int8_t x = (static_cast<int16_t>(newBuffer[1]) << 8) | newBuffer[2];
                int8_t y = (static_cast<int16_t>(newBuffer[3]) << 8) | newBuffer[4];
                int8_t z = (static_cast<int16_t>(newBuffer[5]) << 8) | newBuffer[6];
		uint8_t yaw = newBuffer[7];
		uint8_t pitch = newBuffer[8];
	} else if(new_pack_id == 0x0d){
		uint8_t player_id = 0xFF;
		char message[65];
		message[64] = '\0';

		for(int i=63; i >= 0; i--){
			if(message[i] == ' ' || message[i] == '\0'){
				message[i] = '\0';
			} else{
				break;
			}
		}
	} else{
		cout << "Client sent invalid packet! Closing connection.\n";
		close(clientSocket);
	}

	return 0;
}
