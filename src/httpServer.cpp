#include "httpServer.h"
#include "httpConn.h"
#include "utils/lock.h"

HttpServer::HttpServer(int port)
	: _port(port) {}

HttpServer::~HttpServer()
{
	if (_threadPool) {
		delete _threadPool;
	}
	if (_memPool) {
		delete _memPool;
	}
	closesocket(_sock);
	WSACleanup();
}

bool
HttpServer::init()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("Failed to initialize Winsock.\n");
		return false;
	}

	// connect database
	if (!CGI::init("localhost", "root", "", "mydb", 3306, 10)) {
		std::cout << "cannot connect to database" << std::endl;
		return -1;
	}

	// create thread pool
	_threadPool = new ThreadPool<HTTP::HttpConn>(10);

	// create mem pool
	_memPool = new MemPool<HTTP::HttpConn>(2, 10);
	
	FD_ZERO(&_read_fds);
	FD_ZERO(&_write_fds);

	// init socket 
	_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (_sock < 0) {
		std::cout << "cannot create socket" << std::endl;
		return false;
	}
	sockaddr_in servAddr;
	servAddr.sin_family = AF_INET;
	servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servAddr.sin_port = htons(_port);
	if (bind(_sock, (struct sockaddr*)(&servAddr), sizeof(struct sockaddr_in))) {
		std::cout << "cannot bind socket" << std::endl;
	}
	else {
		std::cout << "webserver bind to: " << _port << " successfully" << std::endl;
	}

	if (listen(_sock, 5)) {
		std::cout << "cannot listen socket" << std::endl;
	}
	else {
		std::cout << _sock << " is listening." << std::endl;
	}

	_clean_thread = std::thread(&HttpServer::_clean_unactive_conn, this);

	return true;
}

void
HttpServer::launch()
{
	int maxfd = _sock;
	fd_set active_rds, active_wts;
	FD_SET(_sock, &_read_fds);
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 1000000;	//Î¢Ãë

	while (true) {
		UTIL::lock(_fdset_lock, "locking fds to select");
		memcpy(&active_rds, &_read_fds, sizeof(fd_set));
		memcpy(&active_wts, &_write_fds, sizeof(fd_set));
		UTIL::unlock(_fdset_lock, "unlocking fds to select");
		int activeCnt = select(maxfd + 1, &active_rds, &active_wts, NULL, &tv);
		if (activeCnt) {
			for (int i = 0; i != maxfd + 1; ++i) {
				if (i == _sock && FD_ISSET(i, &active_rds)) { // no need to lock, because _sock won't tobe modified
					sockaddr client;
					int sockaddr_len = sizeof(client);
					int connfd = accept(i, &client, &sockaddr_len);
					if (connfd < 0) {
						//todo: error out
						std::cout << "failed to accept client" << std::endl;
					}
					else {
						u_long nonBlockingMode = 1;
						if (ioctlsocket(connfd, FIONBIO, &nonBlockingMode) == SOCKET_ERROR) {
							std::cout << "failed to set client non-blocking" << std::endl;
							return;
						}
						std::cout << "accept client successfully" << std::endl;
						HTTP::HttpConn* conn = new (_memPool) HTTP::HttpConn(this, connfd, _memPool);
						_conMap.insert({ connfd, conn });
						_sockMap.insert({conn, connfd});
						UTIL::lock(_fdset_lock, "locking read fds");
						FD_SET(connfd, &_read_fds);
						UTIL::unlock(_fdset_lock, "unlocking read fds");
						if (connfd > maxfd) {
							maxfd = connfd;
						}
					}
					continue;
				}
				if (FD_ISSET(i, &active_rds)) {
					_conMap[i]->recvReq();
					_threadPool->addTask(_conMap[i]);
					_activeMap[i] = UTIL::getTime();
				}
				if (FD_ISSET(i, &active_wts)) {
					_conMap[i]->response();
					_activeMap[i] = UTIL::getTime();
				}
			}
		}
	}
}

void
HttpServer::modify_fd_set(int fd, bool write)
{
	fd_set* src = write ? &_read_fds : &_write_fds;
	fd_set* dest = write ? &_write_fds : &_read_fds;
	UTIL::lock(_fdset_lock, "locking read and write fds");
	FD_SET(fd, dest);
	FD_CLR(fd, src);
	UTIL::unlock(_fdset_lock, "unlocking read and write fds");
}

void 
HttpServer::close_conn(HTTP::HttpConn* conn)
{
	int fd = _sockMap[conn];
	FD_CLR(fd, &_read_fds);
	FD_CLR(fd, &_write_fds);
	_conMap.erase(fd);
	_sockMap.erase(conn);
	delete conn;
}

void
HttpServer::_clean_unactive_conn()
{
	timePoint now = UTIL::getTime();
	for (auto pair : _activeMap) {
		if (UTIL::getmillisec(now, pair.second) > 5000) {
			auto conn = _conMap[pair.first];
			close_conn(conn);
		}
	}
}
