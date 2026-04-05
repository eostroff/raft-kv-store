#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "RaftNode.h"

// Config file format (one line per node):
//   node_id ip port
// Example for 3 nodes:
//   0 127.0.0.1 8000
//   1 127.0.0.1 8001
//   2 127.0.0.1 8002
//
// On Khoury machines, replace 127.0.0.1 with actual hostnames/IPs.

void PrintUsage(char *argv0) {
	std::cout << "Usage: " << argv0 << " <node_id> <config_file>" << std::endl;
	std::cout << std::endl;
	std::cout << "Config file format (one line per node):" << std::endl;
	std::cout << "  node_id ip port" << std::endl;
	std::cout << std::endl;
	std::cout << "Example config for 3 local nodes:" << std::endl;
	std::cout << "  0 127.0.0.1 8000" << std::endl;
	std::cout << "  1 127.0.0.1 8001" << std::endl;
	std::cout << "  2 127.0.0.1 8002" << std::endl;
}

int main(int argc, char *argv[]) {
	if (argc < 3) {
		PrintUsage(argv[0]);
		return 1;
	}

	int my_id = atoi(argv[1]);
	std::string config_file = argv[2];

	// Parse config file
	std::ifstream infile(config_file);
	if (!infile.is_open()) {
		std::cerr << "ERROR: Could not open config file: " << config_file << std::endl;
		return 1;
	}

	int my_port = -1;
	std::vector<PeerInfo> peers;
	std::string line;

	while (std::getline(infile, line)) {
		if (line.empty() || line[0] == '#') continue;

		std::istringstream iss(line);
		int id;
		std::string ip;
		int port;

		if (!(iss >> id >> ip >> port)) {
			std::cerr << "WARNING: Skipping malformed config line: " << line << std::endl;
			continue;
		}

		if (id == my_id) {
			my_port = port;
		} else {
			PeerInfo peer;
			peer.id = id;
			peer.ip = ip;
			peer.port = port;
			peers.push_back(peer);
		}
	}
	infile.close();

	if (my_port < 0) {
		std::cerr << "ERROR: Node id " << my_id << " not found in config file" << std::endl;
		return 1;
	}

	std::cout << "=== Raft Node " << my_id << " ===" << std::endl;
	std::cout << "Listening on port: " << my_port << std::endl;
	std::cout << "Peers:" << std::endl;
	for (auto &p : peers) {
		std::cout << "  Node " << p.id << " at " << p.ip << ":" << p.port << std::endl;
	}
	std::cout << "=========================" << std::endl;

	RaftNode node(my_id, my_port, peers);
	node.Start();

	return 0;
}