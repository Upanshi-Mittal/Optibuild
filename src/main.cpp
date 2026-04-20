#include "auth/auth.h"
#include "billing/billing.h"

int main() {
    signup("user1", "password");
    login("admin", "1234");
    processPayment(100.5);
    return 0;
}