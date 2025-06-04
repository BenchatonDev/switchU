#include <math.h>
#include <string>
#include <nn/act.h>

#include "util.hpp"

std::string ACCOUNT_ID;

void get_user_information() {
    nn::act::Initialize();

    char account_id[256];
    nn::act::GetAccountId(account_id);
    ACCOUNT_ID = std::string(account_id);
}