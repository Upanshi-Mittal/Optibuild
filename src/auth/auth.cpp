#include "auth.h"
#include "../database/database.h"
#include "../utils/utils.h"
#include <iostream>
// test change
bool login(const std::string& username, const std::string& password) {
    if (userExists(username)) {
        std::cout << "Login success for " << username << std::endl;
        return true;
    }
    return false;
}
// 
void signup(const std::string& username, const std::string& password) {
    std::string hashed = hashPassword(password);
    saveUser(username);
    std::cout << "User signed up: " << username << std::endl;
}