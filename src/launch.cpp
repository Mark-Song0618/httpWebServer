#include <iostream>
#include "httpServer.h"

int main(int argc, const char** argv) {
	int webServerPort = 1111;

	if (argc == 2) {
		webServerPort = atoi(argv[1]);
	}
	else if (argc > 2) {
		std::cout << "Usage: " << argv[0] << " <port>" << std::endl;
		return -1;
	}

	HttpServer server(webServerPort);
	if (!server.init()) {
		return -1;
	}

	server.launch();
	return 0;
}