// Here packets that are being received and send immidiately on new connection are defined.
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <cstring>
#include <unistd.h>
#include <zlib.h>

#include "handle_new.hpp"
#include "level.hpp"

using namespace std;

int recvPlayerId(int &clientSocket){
	uint8_t playerIdBuf[131];
	ssize_t bytesReceived = recv(clientSocket, playerIdBuf, sizeof(playerIdBuf) - 1, 0);

	if(bytesReceived < 130){
		cout << "Player id receiving: Buffer error: expected 130 bytes but got " << bytesReceived << endl;
		close(clientSocket);
		return 0;
	}

	struct playerIdPack{
		uint8_t packet_id;
		uint8_t prot_ver;
		string username;
		string ver_key;
		uint8_t unused;
	};

	playerIdPack playerId;
	playerId.packet_id = playerIdBuf[0];
	playerId.prot_ver = playerIdBuf[1];
	playerId.username = string((char*)&playerIdBuf[2], 64);
	playerId.ver_key = string((char*)&playerIdBuf[66], 64);
	playerId.unused = playerIdBuf[130];

	if(playerId.packet_id != 0x00){
		cout << "Player identification packet id ERR: expected packet id with value 0x00 but got 0x" << hex << (int)playerId.packet_id << dec << ". Closing connection.\n";
		close(clientSocket);
	}

	if(playerId.prot_ver != 0x07){
		cout << "Player identification protocol version ERR: expected protocol version 0x07 but got 0x" << hex << (int)playerId.prot_ver << dec << ". Closing connection.\n";
		close(clientSocket);
	}

	cout << "New connection fully identified. Welcome " << playerId.username << " to the server!\n";

	return 0;
}

int sendServerId(int &clientSocket, string name, string motd){
	// SERVER IDENTIFICATION
	uint8_t serverIdBuf[131];
	struct serverIdPack{
		uint8_t packet_id;
		uint8_t prot_ver;
		char name[64];
		char motd[64];
		uint8_t usrtype;
	};
	serverIdPack serverId;
	serverId.packet_id = 0x00;
	serverId.prot_ver = 0x07;
	serverId.usrtype = 0x00;

	serverIdBuf[0] = serverId.packet_id;
	serverIdBuf[1] = serverId.prot_ver;

	// the rest is filled with 0x20 (ASCII spaces) instead of null-terminators (0x00)
	// for stability across both Notch's original Minecraft 0.30c (which crashes on 0x00s, accepts only 0x20s) and ClassiCube (allows both 0x00s and 0x20s)
	memset(&serverIdBuf[2], 0x20, 64);
	memset(&serverIdBuf[66], 0x20, 64);

	memcpy(&serverIdBuf[2], name.c_str(), name.length());
	memcpy(&serverIdBuf[66], motd.c_str(), motd.length());

	serverIdBuf[130] = serverId.usrtype;
	    
	if(send(clientSocket, serverIdBuf, sizeof(serverIdBuf), 0) < 0){
		cout << "Send server id: Error: failed to send server identification packet. This is fatal to the connection. It will be closed.\n";
		close(clientSocket);
	}

	return 0;
}

void sendLevelInit(int &clientSocket){
	uint8_t level_init_packet = 0x02;
	send(clientSocket, &level_init_packet, sizeof(level_init_packet), 0);
}

void sendLevelData(int &clientSocket){
	uint8_t levelDataBuf[1028];
	struct __attribute__((packed)) levelDataPack{
		uint8_t packet_id;
		short chunk_length;
		uint8_t chunk_data[1024];
		uint8_t percent_complete;
	};

	levelDataPack levelData;

	level_x = 64, level_y = 64, level_z = 64;
	int total_blocks = level_x * level_y * level_z;
	uint8_t* level_blocks = new uint8_t[total_blocks];

	for(int y = 0; y < level_y; y++){
		for(int z = 0; z < level_z; z++){
			for(int x = 0; x < level_x; x++){
				int index = (y * level_z + z) * level_x + x;

				if(y == 31)
					level_blocks[index] = 0x02;
				else if(y < 31)
					level_blocks[index] = 0x03;
				else
					level_blocks[index] = 0x00;
			}
		}
	}

	int prefixed_size = 4 + total_blocks;
	uint8_t* prefixed_data = new uint8_t[prefixed_size];
	prefixed_data[0] = (total_blocks >> 24) & 0xFF;
	prefixed_data[1] = (total_blocks >> 16) & 0xFF;
	prefixed_data[2] = (total_blocks >> 8) & 0xFF;
	prefixed_data[3] = total_blocks & 0xFF;
	memcpy(prefixed_data + 4, level_blocks, total_blocks);

	uLongf compressed_size = compressBound(prefixed_size);
	uint8_t* compressed_data = new uint8_t[compressed_size];

	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;
	deflateInit2(&stream, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);

	stream.avail_in = prefixed_size;
	stream.next_in = prefixed_data;
	stream.avail_out = compressed_size;
	stream.next_out = compressed_data;

	deflate(&stream, Z_FINISH);
	compressed_size = stream.total_out;
	deflateEnd(&stream);

	int chunks = (compressed_size + 1023) / 1024;
	for(int chunk = 0; chunk < chunks; chunk++){
		levelData.packet_id = 0x03;

		int remaining = compressed_size - (chunk * 1024);
		int chunk_size = (remaining < 1024) ? remaining : 1024;
		levelData.chunk_length = htons(chunk_size);

		memcpy(levelData.chunk_data, compressed_data + (chunk * 1024), chunk_size);

		if(chunk_size < 1024){
			memset(levelData.chunk_data + chunk_size, 0x00, 1024 - chunk_size);
		}

		levelData.percent_complete = (uint8_t)((chunk + 1) * 100 / chunks);

		memcpy(levelDataBuf, &levelData, sizeof(levelData));
		send(clientSocket, levelDataBuf, sizeof(levelDataBuf), 0);
	}

	delete[] level_blocks;
	delete[] prefixed_data;
	delete[] compressed_data;
}

void sendLevelFinalize(int &clientSocket){
	uint8_t levelFinalizeBuf[7];

	struct __attribute__((packed)) levelFinalizePack{
		uint8_t packet_id;
		short x, y, z;
	};

	levelFinalizePack levelFinalize;
	levelFinalize.packet_id = 0x04;
	levelFinalize.x = htons(64); // FIX: make it synchronized with sendLevelData's already declared map boundaries
	levelFinalize.y = htons(64);
	levelFinalize.z = htons(64);

	memcpy(levelFinalizeBuf, &levelFinalize, sizeof(levelFinalize));
	send(clientSocket, levelFinalizeBuf, sizeof(levelFinalizeBuf), 0);	
}

