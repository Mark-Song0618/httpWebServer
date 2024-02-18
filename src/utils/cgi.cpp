#include "cgi.h"
#include "conPool.h"
#include <map>
#include <iostream>

namespace CGI {
#define FREE_RES(conn) mysql_free_result(mysql_store_result(conn));
static ConnPool* _connPool = nullptr;

static std::map<std::string, std::string> _register_table;

static bool
_load_register_table()
{
	MYSQL* conn = _connPool->getConn();
	bool rt = true;
	MYSQL_RES* result;

	if (!conn) {
		return false;
	}
	// 1.查询表是否存在，不存在则建表并返回 true
	if (mysql_query(conn, "SHOW TABLES LIKE 'users'")) {
		std::cout << "Error failed to query database: " << mysql_error(conn) << std::endl;
		FREE_RES(conn)
		_connPool->releaseConn(conn);
		return false;
	}
	else {
		result = mysql_store_result(conn);
	}

	if (mysql_num_rows(result) == 0) {
		// create table
		FREE_RES(conn)
		if (mysql_query(conn, "CREATE TABLE users(Id INT PRIMARY KEY AUTO_INCREMENT, name VARCHAR(20), pw VARCHAR(20))"))
		{
			std::cout << "Error create table to database: " << mysql_error(conn) << std::endl;
			rt = false;
		}
		FREE_RES(conn)
		_connPool->releaseConn(conn);
		return rt;
	}

	// 2.若表存在，则查询所有的记录
	FREE_RES(conn)
	if (mysql_query(conn, "SELECT name, pw FROM users"))
	{
		std::cout << "Error query to database: " << mysql_error(conn) << std::endl;
		_connPool->releaseConn(conn);
		FREE_RES(conn)
		return false;
	}
	else {
		result = mysql_store_result(conn);
	}
	
	if (result == NULL)
	{
		std::cout << "Bad results: " << mysql_error(conn) << std::endl;
	}

	int num_fields = mysql_num_fields(result);

	MYSQL_ROW row;

	while ((row = mysql_fetch_row(result)))
	{
		_register_table[row[0]] = row[1];
	}

	mysql_free_result(result);
	_connPool->releaseConn(conn);
	return true;
}

bool init(std::string url, std::string User, std::string PassWord, std::string DataBaseName, int Port, size_t maxConn)
{
	if (!_connPool) {
		_connPool = new ConnPool(url, User, PassWord, DataBaseName, Port, maxConn);
	}

	return _load_register_table();
}

bool user_register(std::string name, std::string pw)
{
	if (pw.empty() || name.empty()) {
		return false;
	}
	
	// 1.保存记录到_register_table
	if (_register_table.find(name) != _register_table.end()) {
		return false;
	}
	else {
		_register_table[name] = pw;
	}
	
	// 2.添加信息到namedb
	MYSQL* conn = _connPool->getConn();
	if (!conn) {
		_connPool->releaseConn(conn);
		return false;
	}
	std::string msg("INSERT INTO users(name, pw)");
	msg += "VALUES('";
	msg += name;
	msg += "','";
	msg += pw;
	msg += "')";
	bool rt = mysql_query(conn, msg.c_str());
	_connPool->releaseConn(conn);
	return rt;
}

bool is_valid_user(std::string name, std::string pw)
{
	if (name.empty() || pw.empty()) {
		return false;
	}
	if (_register_table.find(name) == _register_table.end()) {
		return false;
	}
	if (_register_table[name] != pw) {
		return false;
	}
	return true;
}

}