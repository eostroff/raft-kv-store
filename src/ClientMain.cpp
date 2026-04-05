#include <iostream>

// Placeholder client for PR1
// PR2 will add GET/PUT/DELETE commands to the Raft cluster

int main(int argc, char *argv[]) {
	if (argc < 3) {
		std::cout << "Usage: " << argv[0] << " [leader_ip] [leader_port]" << std::endl;
		return 0;
	}

	std::string ip = argv[1];
	int port = atoi(argv[2]);

	std::cout << "Raft KV Client" << std::endl;
	std::cout << "Connecting to " << ip << ":" << port << std::endl;
	std::cout << "(Not yet implemented - coming in PR2)" << std::endl;

	return 0;
}