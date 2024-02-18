#include <iostream>
#include <WinSock2.h>
#include <sstream>
#pragma comment(lib, "ws2_32.lib")

int main(int argc, const char** argv) {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		printf("Failed to initialize Winsock.\n");
		return 1;
	}
	int serverPort = 1111;
	/*
	if (argc != 2) {
		std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
		return -1;
	}
	else {
		serverPort = atoi(argv[1]);
	}
	*/

	int clientfd = socket(AF_INET, SOCK_STREAM, 0);
	sockaddr_in serverAddr;
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	serverAddr.sin_port = htons(serverPort);
	std::cout << "connecting to " << serverAddr.sin_addr.s_addr << ":" << serverPort << " ..." << std::endl;
	if (!connect(clientfd, (sockaddr*)&serverAddr, sizeof(sockaddr))) {
		std::cout << "connect to server successfully." << std::endl;
	}
	else {
		std::cout << "failed to connect to server.";
		return -2;
	}

	// get
	std::stringstream get_msg;
	get_msg << "GET /index.html HTTP/1.1" << std::endl;
	get_msg << "Host: www.example.com" << std::endl;
	get_msg << "User-Agent: Mozilla/5.0" << std::endl;	
	int get_len = send(clientfd, get_msg.str().c_str(), get_msg.str().size(), NULL);
	std::cout << "sended: " << get_len << "B" << std::endl;

	// post
	std::stringstream post_msg;
	post_msg << "POST /submit_form HTTP/1.1" << std::endl;
	post_msg << "Host: www.example.com" << std::endl;
	post_msg << "User-Agent: Mozilla/5.0" << std::endl;
	post_msg << "Content-Type: application/x-www-form-urlencoded" << std::endl;
	post_msg << "Content-Length: 25" << std::endl;
	post_msg << std::endl;
	post_msg << "username=user&password=pass" << std::endl;
	int post_len = send(clientfd, post_msg.str().c_str(), post_msg.str().size(), NULL);
	std::cout << "sended: " << post_len << "B" << std::endl;

	getchar();
	return 0;
}