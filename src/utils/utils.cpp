#include "utils.h"

std::string hashPassword(const std::string& password) {
    return "hashed_" + password;
}