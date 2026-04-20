#include "../src/auth/auth.h"
#include <cassert>

int main() {
    signup("test", "123");
    assert(login("admin", "123") == true);
    return 0;
}