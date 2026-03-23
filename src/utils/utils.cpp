#include "utils.h"
#include <thread>
#include <chrono>

std::string getUtilMessage() {
    std::this_thread::sleep_for(std::chrono::milliseconds(100)); // simulate work
    return "Utils Ready";
}