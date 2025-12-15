#ifndef HEARTBEAT_HPP
#define HEARTBEAT_HPP

#include <string>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
bool sendHeartbeat(const std::string& name, int port, int users, int max, const std::string& salt, bool isPublic);

#endif
