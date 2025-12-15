#include <curl/curl.h>
#include <iostream>
#include <string>

#include "heartbeat.hpp"
using namespace std;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

bool sendHeartbeat(const string& name, int port, int users, int max, const string& salt, bool isPublic, const string& software, bool web) {
	CURL* curl = curl_easy_init();
	if(!curl) return false;

	string response;
	string url = "http://www.classicube.net/server/heartbeat?name=" + name + 
		"&port=" + to_string(port) + 
		"&users=" + to_string(users) + 
		"&max=" + to_string(max) + 
		"&salt=" + salt + 
		"&public=" + (isPublic ? "true" : "false") +
		"&software=" + software +
		"&web=" + (web ? "true" : "false");

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

	CURLcode res = curl_easy_perform(curl);
	curl_easy_cleanup(curl);

	if(res == CURLE_OK) {
		cout << "Heartbeat sent. Result: " << response << endl;
		return true;
	}
	return false;
}

