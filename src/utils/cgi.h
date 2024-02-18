#pragma once
#include <string>

namespace CGI {
	bool init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, size_t maxConn);
	bool user_register(std::string name, std::string pw);
	bool is_valid_user(std::string name, std::string pw);
}