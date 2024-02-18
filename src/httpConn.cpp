#include "httpConn.h"
#include <iostream>
#include <winsock2.h>
#include <fstream>
#include "utils/cgi.h"

#pragma comment(lib, "ws2_32.lib")

using namespace HTTP;

int
HttpReqFSM::parse(char* begin, char* end, HTTPCODE& rt)
{
	LINE_STATUS line_status;
	char* line_tail;
	int parsed_len = 0;
	// 从给定的范围里解析request，返回解析的长度以及http状态码
	while (begin != end) {
		line_status = _getLine(begin, end, line_tail);
		if (line_status == LINE_OPEN) {
			// need more input
			rt = NO_REQUEST;
			return parsed_len;
		} else if (line_status == LINE_BAD) {
			rt = BAD_REQUEST;
			return parsed_len;
		}
		else {
			// tail is '\n'
			switch (_status)
			{
			case PARSE_REQ_LINE:
				rt = _parse_req_line(begin, line_tail);
				if (rt == GET_REQUEST) {
					parsed_len += (line_tail - begin);
					begin = line_tail+1;
					_status = PARSE_HEADER;
				}
				else {
					return rt;
				}
			case PARSE_HEADER:
				rt = _parse_req_header(begin, line_tail);
				if (rt == GET_REQUEST) {
					parsed_len += (line_tail - begin);
					begin = line_tail+1;
					if (_result.post_len > 0) {
						if (_result.post_len + begin <= end) {
							// content is all here
							_result.post_content = begin;
							*(_result.post_content + _result.post_len) = '\0';
							parsed_len += _result.post_len;
							rt = FILE_REQUEST;
						}
						else {
							// wait for more data
							rt = NO_REQUEST;
						}
						return parsed_len;
					}
					else {
						rt = FILE_REQUEST;
						return parsed_len;
					}	
				}
				else if (rt == BAD_REQUEST) {
					return rt;
				}
				else {
					parsed_len += (line_tail - begin);
					begin = line_tail + 1;
					continue;	// header has more lines
				}
			default:
				break;
			}
		}
	}

	return parsed_len;
}

void
HttpReqFSM::reset()
{
	_status = PARSE_REQ_LINE;
	_result.post_len = 0;
	_result.post_content = nullptr;
	_unchecked = nullptr;
	_result.url = nullptr;
	_result.method = nullptr;
	_result.version = nullptr;
}

// 从一段文本中找到一整行内容，返回状态及行位位置
HttpReqFSM::LINE_STATUS
HttpReqFSM::_getLine(char*& begin, char*& end, char*& tail)
{
	for (char* iter = begin; iter != end; ++iter) {
		if (*iter == '\r') {
			if (iter == end - 1) {
				return LINE_OPEN;
			}
			else if (*(iter + 1) == '\n') {
				tail = iter + 1;
				return LINE_OK;
			}
			else {
				return LINE_BAD;
			}
		}
	}
	return LINE_OPEN;
}

/*
	@brief	parse request info between [begin, end)
	@param  buffer range
	@return
		BAD_REQUEST : if not parsed successfully
		GET_REQUEST : parsed successfully
*/
HTTPCODE
HttpReqFSM::_parse_req_line(char* begin, char* end)
{
	char* iter = begin;
	while (isspace(*iter) && (iter != end)) ++iter;
	if (iter == end) {
		return BAD_REQUEST;
	}
	if (strncmp(iter, "GET", 3) == 0) {
		*(iter + 3) = '\0';
		_result.method = iter;
		iter += 4;
	}
	else if (strncmp(iter, "POST", 4) == 0) {
		*(iter + 4) = '\0';
		_result.method = iter;
		iter += 5;
	}
	else {
		return BAD_REQUEST;
	}
	
	while (isspace(*iter) && (iter != end)) ++iter;
	if (iter == end) {
		return BAD_REQUEST;
	}
	if (*iter == '/') {
		char* tail = iter;
		while (!isspace(*tail) && (iter != end)) ++tail;
		if (iter == end) {
			return BAD_REQUEST;
		}
		*tail = '\0';
		_result.url = iter;
		iter = tail + 1;
	}
	else {
		return BAD_REQUEST;
	}

	while (isspace(*iter) && (iter != end)) ++iter;
	if (iter == end) {
		return BAD_REQUEST;
	}
	if (strncmp(iter, "HTTP", 4) == 0) {
		_result.version = iter;
		while (!isspace(*iter) && (iter < end)) ++iter;
		if (iter < end) *iter = '\0';
		*end = '\0';
		*(end - 1) = '\0';
	}
	else {
		return BAD_REQUEST;
	}

	return GET_REQUEST;
}

/*
	@brief	parse request header from [begin, end) 
	@param  buffer range
	@return
		BAD_REQUEST : if not parsed successfully
		GET_REQUEST : parsed successfully(begin is \r\n)
		NO_REQUEST  : not finished
*/
HTTPCODE
HttpReqFSM::_parse_req_header(char* begin, char* end)
{
	char* iter = begin;
	if (*iter == '\r' && *(iter + 1) == '\n') {
		return GET_REQUEST;
	}
	if (strncmp(iter, "Content-Length:", 15) == 0) {
		while (!isdigit(*iter) ) ++iter;
		_result.post_len = atoi(iter);
		return NO_REQUEST;
	}
	else {
		// ignore other info Temporarily
		return NO_REQUEST;
	}
}

bool
HttpResponser::response(HTTPCODE code, HttpReqData& data, char* output, int maxSize)
{
	switch (code)
	{
	case HTTP::BAD_REQUEST:
		_response_bad_request(output, maxSize);
		break;
	case HTTP::NO_RESOURCE:
		_response_no_res(output, maxSize);
		break;
	case HTTP::FORBIDDEN_REQUEST:
		_response_forbid(output, maxSize);
		break;
	case HTTP::FILE_REQUEST:
		_response_file_request(data, output, maxSize);
		break;
	case HTTP::INTERNAL_ERROR:
		_response_in_error(output, maxSize);
		break;
	case HTTP::CLOSED_CONNECTION:
		_response_closing(output, maxSize);
		break;
	default:
		break;
	}
	return true;
}

std::string 
HttpResponser::_getResponseFile(HttpReqData& data)
{
	// 1. if conn, return mainpage
	if (strncmp(data.method, "GET", 3) == 0) {
		return MAIN_PAGE;
	}

	// 2. get pw and name
	std::string name, pw;
	char* iter = data.post_content;
	if (strncmp(iter, "username=", 9) != 0) {
		return "";
	}

	while (*iter != '&' && (iter - data.post_content != data.post_len)) {
		++iter;
	}
	if (iter - data.post_content == data.post_len) {
		return "";
	}
	else {
		name = std::string(data.post_content + 9, (++iter - 1) - data.post_content - 9);
	}

	if (strncmp(iter, "password=", 9) == 0) {
		iter += 9;
		pw = std::string(iter, data.post_len - (name.size() + 19));
	}
	else {
		return  "";
	}
	// 2. if register
	if (strncmp(data.url, "/register", 9) == 0) {
		CGI::user_register(name, pw);
		return WELCOME_PAGE;
	}
	else if (strncmp(data.url, "/login", 6) == 0) {
		if (CGI::is_valid_user(name, pw)) {
			return WELCOME_PAGE;
		}
	}
	return "";
}

bool
HttpResponser::_response_file_request(HttpReqData& data, char* output, int maxSize)
{
	std::string rel_filePath = _getResponseFile(data);
	if (rel_filePath.empty()) {
		return 	_response_forbid(output, maxSize);
	}
	memcpy(output,		"HTTP/1.1 200 OK\r\n", 17);
	memcpy(output + 17, "Connection: close\r\n", 19);
	memcpy(output + 36, "Content-Type: text/html\r\n", 25);
	memcpy(output + 61,	"Content-Length: 8192\r\n\r\n", 24);
	int pos = 85;
	std::string abs_filePath = std::string(HTML_ROOT) + rel_filePath;
	std::ifstream file(abs_filePath.c_str());
	std::string content;
	while (std::getline(file, content)) {
		memcpy(output + pos, content.c_str(), content.size());
		pos += content.size();
	}
	file.close();
	return true;
}

bool
HttpResponser::_response_bad_request(char* output, int maxSize)
{
	memcpy(output, "HTTP/1.1 400 Bad Request\r\n", 26);
	memcpy(output + 26, "Connection: close\r\n", 19);
	return true;
}

bool
HttpResponser::_response_no_res(char* output, int maxSize)
{
	memcpy(output, "HTTP/1.1 404 Not Found\r\n", 24);
	memcpy(output + 24, "Connection: close\r\n", 19);
	return true;
}

bool
HttpResponser::_response_forbid(char* output, int maxSize)
{
	memcpy(output, "HTTP/1.1 403 Forbidden\r\n", 24);
	memcpy(output + 24, "Connection: close\r\n", 19);
	memcpy(output + 43, "Content-Type: text/html\r\n", 25);
	memcpy(output + 68, "Content-Length: 8192\r\n\r\n", 24);
	int pos = 92;
	std::string abs_filePath = std::string(HTML_ROOT) + FORBID_PAGE;
	std::ifstream file(abs_filePath);
	std::string content;
	while (std::getline(file, content)) {
		memcpy(output + pos, content.c_str(), content.size());
		pos += content.size();
	}
	file.close();
	return true;
}

bool
HttpResponser::_response_in_error(char* output, int maxSize)
{
	memcpy(output, "HTTP/1.1 500 Internal Server Error\r\n", 36);
	memcpy(output + 36, "Connection: close\r\n", 19);
	return true;
}

bool
HttpResponser::_response_closing(char* output, int maxSize)
{
	return true;
}

HttpConn::HttpConn(HttpServer* server, int sock, MemPool<HttpConn>* memPool)
	: _server(server), _client(sock), _memPool(memPool) {}

void*
HttpConn::operator new (size_t size, MemPool<HttpConn>* pool)
{
	//std::cout << "alloc from memPool" << std::endl;
	return pool->alloc();
}

void 
HttpConn::operator delete(void* ptr)
{
	HttpConn* con = (HttpConn*)ptr;
	if (con->_memPool) {
	//	std::cout << "free in memPool" << std::endl;
		con->_memPool->free(con);
		con->~HttpConn();
	}
	else {	
		::operator delete(con);
	}
}

void
HttpConn::run()
{
	// 解析request,保存有效字段
	HTTPCODE status = _parse_request();
	if (status == NO_REQUEST) {
		return;
	}
	
	// 根据request 生成response报文
	HttpReqData& data = _get_request_result();
	if (!_prepare_response(status, data)) {
		_server->close_conn(this);
	}
	// response has been writen to wt_buf, rd_buf is useless
	_clean_read();

	_modify_fd_set();
}

void
HttpConn::recvReq()
{
	std::string log = "locking read buffer for ";
	log += std::to_string(_client);
	UTIL::lock(_rd_lock, log.c_str());
	int total = 0;
	while (true) {
		int once = recv(_client, _rd_buf + _rd_buf_idx, MAX_READ_BYTE - _rd_buf_idx, 0);
		if (once > 0) {
			total += once;
			_rd_buf_idx += once;
		}
		else {
			break;
		}
	}
	if (total == 0) {
		UTIL::unlock(_rd_lock, "unlocking read buffer");
		return;
	}
	char msg[MAX_READ_BYTE + 1];
	memcpy(msg, _rd_buf + _rd_buf_idx - total, total);
	_idle = false;
	UTIL::unlock(_rd_lock, "unlocking read buffer");
	msg[total + 1] = '\0';
	std::cout << "socket: " << _client << " received:\r\n" << msg << std::endl;
}

HTTPCODE
HttpConn::_parse_request()
{
	HTTPCODE rt;
	std::string log = "locking read buffer to parse";
	log += std::to_string(_client);
	UTIL::lock(_rd_lock, log.c_str());
	char* end = _rd_buf + _rd_buf_idx;	// this could be modified by readable event
	UTIL::unlock(_rd_lock, "unlocking read buffer");
	int len = _fsm.parse(_rd_buf + _rd_buf_unchecked_idx, end, rt);
	_rd_buf_unchecked_idx += len;
	return rt;
}

bool
HttpConn::_prepare_response(HTTPCODE code, HttpReqData& data)
{
	HttpResponser responser;
	return responser.response(code, data, _wt_buf);
}

void 
HttpConn::response()
{
	send(_client, _wt_buf, MAX_WRITE_BYTE, 0);
	_idle = true;
	_modify_fd_set(false);
}

void
HttpConn::_modify_fd_set(bool toWrite)
{
	_server->modify_fd_set(_client, toWrite);
}

void
HttpConn::_clean_read() {
	memset(_rd_buf, '\0', MAX_READ_BYTE);
	_rd_buf_idx = 0;
	_rd_buf_unchecked_idx = 0;
	_fsm.reset();
}