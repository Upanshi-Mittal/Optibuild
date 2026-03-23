#include "auth.h"
#include "../database/db.h"

std::string authenticate() {
    return "Auth Success using " + connectDB();
}