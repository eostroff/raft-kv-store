#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "RaftNode.h"

int main(int argc, char *argv[]) {
	int my_port;
	int my_id;
	int num_peers;
	std::vector<PeerInfo> peers;

	if (argc < 4) {
		std::cout << "not enough arguments" << std::endl;
        std::cout << argv[0] << " [port #] [node id] [num peers] [peer1_id peer1_ip peer1_port] ..." << std::endl;
		return 1;
	}

	my_port = atoi(argv[1]);
	my_id = atoi(argv[2]);
	num_peers = atoi(argv[3]);

	if (argc != 4 + (num_peers * 3)) {
		std::cerr << "argument error for peer list" << std::endl;
		return 1;
	}

	for (int i = 0; i < num_peers; i++) {
		int base = 4 + (i * 3);
		PeerInfo peer;
		peer.id = atoi(argv[base]);
		peer.ip = argv[base + 1];
		peer.port = atoi(argv[base + 2]);

		if (peer.id == my_id) {
			std::cerr << "ERROR: Peer id " << peer.id << " matches local node_id" << std::endl;
			return 1;
		}
		if (peer.port <= 0) {
			std::cerr << "ERROR: Peer port must be a positive integer for peer id "
				<< peer.id << std::endl;
			return 1;
		}

		peers.push_back(peer);
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