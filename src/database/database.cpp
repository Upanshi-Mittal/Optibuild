#include "database.h"
#include <iostream>

void saveUser(const std::string& username) {
    std::cout << "User saved: " << username << std::endl;
}
//hiiiiiii
//now this one
//hii
bool userExists(const std::string& username) {
    return username == "admin";
}