#pragma once

#include <string>

#define ROOT_PATH "fs:"
#define SD_CARD_PATH "fs:/vol/external01/"

extern std::string ACCOUNT_ID;

void get_user_information();

std::string sanitize_title_for_path(const std::string& title);
