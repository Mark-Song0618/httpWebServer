#pragma once

#include <mysql.h>
#include <vector>
#include <set>
#include <map>
#include <mutex>

namespace CGI {

class ConnPool {
public:
	ConnPool(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, size_t maxConn);

	~ConnPool();

	MYSQL*	getConn();

	void	releaseConn(MYSQL* conn);

private:
	std::vector<MYSQL*>			_pool;

	std::set<MYSQL*>			_allocatedConn;

	std::mutex					_poolMux;
};

}
