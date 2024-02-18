#pragma once
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <chrono>
#include <map>
#include <winsock2.h>
#include <mutex>
#include "utils/memPool.h"
#include "utils/threadPool.h"
#include "utils/cgi.h"
#include "utils/gettime.h"
namespace HTTP {
	class HttpConn;
};

class HttpServer {
public:
	HttpServer(int port);

	~HttpServer();

	bool init();

	void launch();

	void modify_fd_set(int fd, bool write = true);

	void close_conn(HTTP::HttpConn*);

private:
	void	_clean_unactive_conn();

private:
	ThreadPool<HTTP::HttpConn>*	_threadPool;

	MemPool<HTTP::HttpConn>*	_memPool;

	fd_set _read_fds;
	
	fd_set _write_fds;

	int _sock;

	int	_port;

	std::mutex	_fdset_lock;

	std::thread _clean_thread;

	std::map<unsigned, HTTP::HttpConn*> _conMap;	// socket fd : HttpConn*

	std::map<HTTP::HttpConn*, unsigned> _sockMap;	// HttpConn* : socket fd

	std::map<unsigned, timePoint>		_activeMap; // socket fd : timestamp
};
