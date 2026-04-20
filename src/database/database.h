#ifndef DATABASE_H
#define DATABASE_H

#include <string>

void saveUser(const std::string& username);
bool userExists(const std::string& username);

#endif