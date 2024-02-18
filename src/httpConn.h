#pragma once

#include "httpServer.h"
#include "utils/lock.h"

#define MAX_READ_BYTE 1024
#define MAX_WRITE_BYTE 16384

#define HTML_ROOT			"D:\\MyProjects\\GitHubProjects\\webServer\\src\\root\\"
#define MAIN_PAGE			"pages\\mainPage.html"
#define WELCOME_PAGE		"pages\\welcome.html"
#define FORBID_PAGE			"pages\\forbidden.html"

namespace HTTP {

enum HTTPCODE {
	NO_REQUEST,			// request not parsed
	GET_REQUEST,		// request is get
	BAD_REQUEST,		// not a valid request
	NO_RESOURCE,		// no required source
	FORBIDDEN_REQUEST,	// request not permitted
	FILE_REQUEST,		// the file of url is requried
	INTERNAL_ERROR,		// server internal error
	CLOSED_CONNECTION	// connection has been closed
};

typedef struct {
	char* url;
	char* post_content;
	char* method;
	char* version;
	int   post_len;
} HttpReqData;

class HttpReqFSM {
public:
	HttpReqFSM() : _status(PARSE_REQ_LINE) {};

	int   parse(char* begin, char* end, HTTPCODE& rt);

	void  reset();

	HttpReqData& get_result() { return _result; }

private:
	enum STATUS {
		PARSE_REQ_LINE,
		PARSE_HEADER,
		PARSE_BODY,
	};
	enum LINE_STATUS {
		LINE_OK,
		LINE_OPEN,
		LINE_BAD
	};

	LINE_STATUS _getLine(char* &begin, char* &end, char* &tail);

	HTTPCODE	_parse_req_line(char* begin, char* end);

	HTTPCODE	_parse_req_header(char* begin, char* end);

private:
	STATUS	_status;

	HttpReqData _result;

	char*	_unchecked;
};

class HttpResponser {
public:
	bool response(HTTPCODE code, HttpReqData& data, char* output, int maxSize = MAX_WRITE_BYTE);

private:
	bool	_response_file_request(HttpReqData&data, char* output, int maxSize);

	bool	_response_bad_request(char* output, int maxSize);

	bool	_response_no_res(char* output, int maxSize);

	bool	_response_forbid(char* output, int maxSize);

	bool	_response_in_error(char* output, int maxSize);

	bool	_response_closing(char* output, int maxSize);

	std::string _getResponseFile(HttpReqData& data);
};

class HttpConn {
public:
	HttpConn(HttpServer* server, int client = -1, MemPool<HttpConn>* memPool = nullptr);

	void* operator new (size_t, MemPool<HttpConn>*);

	void operator delete(void*);

	void recvReq();	 // read data from socket to read_buf

	void response();

	void run();		 // worker of thread

	bool isIdle() { return _idle; }

private:
	HTTPCODE _parse_request();

	bool _prepare_response(HTTPCODE, HttpReqData& url);

	void set_idle(bool idle) { _idle = idle; }
	
	void _modify_fd_set(bool toWrite = true);

	HttpReqData& _get_request_result() { return _fsm.get_result(); }

	void _clean_read();

private:
	MemPool<HttpConn>* _memPool;

	HttpReqFSM		_fsm;		// parse request

	HttpResponser	_responser;	// response to request

	HttpServer*     _server;

	int	_rd_buf_idx = 0;		// the index of the next unused byte in rd_buf
	int _rd_buf_unchecked_idx = 0;
	
	char _rd_buf[MAX_READ_BYTE];
	char _wt_buf[MAX_WRITE_BYTE];
	std::mutex	_rd_lock;

	bool _idle = true;

	int	 _client;
};

}