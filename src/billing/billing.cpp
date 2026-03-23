#include "billing.h"
#include "../auth/auth.h"

std::string processPayment() {
    return "Payment Done after " + authenticate();
}