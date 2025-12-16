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

using namespace std;
