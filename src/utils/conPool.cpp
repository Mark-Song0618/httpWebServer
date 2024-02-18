#include <string>
#include <iostream>
#include <mutex>
#include "conPool.h"

namespace CGI {

ConnPool::ConnPool(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, size_t maxConn)
{
	for (int i = 0; i < maxConn; ++i) {
		_pool.push_back(new MYSQL());
		mysql_init(_pool[i]);
		if (!mysql_real_connect(_pool[i], url.c_str(), User.c_str(), PassWord.c_str(), DataBaseName.c_str(), Port, NULL, 0)) {
			std::cout << "Error connecting to database: " << mysql_error(_pool[i]) << std::endl;
			mysql_close(_pool[i]);
			throw std::exception();
		}
	}
}


ConnPool::~ConnPool()
{
	for (auto& conn : _pool) {
		mysql_close(conn);
		delete conn;
	}
}

MYSQL*
ConnPool::getConn()
{
	std::lock_guard<std::mutex> lock(_poolMux);
	for (auto& conn : _pool) {
		if (_allocatedConn.find(conn) == _allocatedConn.end()) {
			_allocatedConn.insert(conn);
			return conn;
		}
	}
	return NULL;
}

void
ConnPool::releaseConn(MYSQL* conn)
{
	std::lock_guard<std::mutex> lock(_poolMux);
	_allocatedConn.erase(conn);
}

}
