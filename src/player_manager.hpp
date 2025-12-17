#ifndef PLAYER_MANAGER_HPP
#define PLAYER_MANAGER_HPP

#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>

struct Player{
	int clientSocket;
	uint8_t playerId;
	std::string username;
	int16_t x, y, z;
	uint8_t yaw, pitch;
	std::thread playerThread;
	std::atomic<bool> active;

	Player(int socket, uint8_t id, std::string name)
		: clientSocket(socket), playerId(id), username(name),
		x(0), y(0), z(0), yaw(0), pitch(0), active(true) {}
};

class PlayerManager {
	public:
		std::vector<Player*> players;
		std::mutex playersMutex;

	private:
		std::atomic<uint8_t> nextPlayerId;

	public:
		PlayerManager();
		~PlayerManager();
		
		uint8_t addPlayer(int clientSocket, std::string username);
		void removePlayer(uint8_t playerId);
		void broadcastToOthers(uint8_t senderPlayerId, const uint8_t* data, size_t length);
		void broadcastToAll(const uint8_t* data, size_t length);
		void sendToPlayer(uint8_t playerId, const uint8_t* data, size_t length);
		Player* getPlayer(uint8_t playerId);
		void updatePlayerPosition(uint8_t playerId, int16_t x, int16_t y, int16_t z, uint8_t yaw, uint8_t pitch);
}

extern PlayerManager* g_playerManager;

void playerGameLoop(int clientSocket, uint8_t playerId);

#endif
