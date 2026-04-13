#include <cstdlib>
#include <iostream>
#include <string>

#include "RaftStub.h"

int main(int argc, char *argv[]) {
	if (argc < 5) {
		printf("not enough arguments\n");
		printf("usage: %s [server_ip] [server_port] [GET/PUT/DELETE] [key] [value (for PUT)]\n", argv[0]);
		return 1;
	}

	std::string ip = argv[1];
	int port = atoi(argv[2]);
	std::string op = argv[3];

	ClientCommand cmd;
	if (op == "GET") {
		if (argc != 5) {
			printf("not enough arguments for GET\n");
			printf("usage: %s [server_ip] [server_port] GET [key]\n", argv[0]);
			return 1;
		}
		cmd.SetCommand(CLIENT_OP_GET, argv[4], "");
	} else if (op == "PUT") {
		if (argc != 6) {
			printf("not enough arguments for PUT\n");
			printf("usage: %s [server_ip] [server_port] PUT [key] [value]\n", argv[0]);
			return 1;
		}
		cmd.SetCommand(CLIENT_OP_PUT, argv[4], argv[5]);
	} else if (op == "DELETE") {
		if (argc != 5) {
			printf("not enough arguments for DELETE\n");
			printf("usage: %s [server_ip] [server_port] DELETE [key]\n", argv[0]);
			return 1;
		}
		cmd.SetCommand(CLIENT_OP_DELETE, argv[4], "");
	} else {
		printf("invalid operation\n");
		printf("usage: %s [server_ip] [server_port] [GET/PUT/DELETE] [key] [value (for PUT)]\n", argv[0]);
		return 1;
	}

	for (int attempt = 0; attempt < 3; ++attempt) {
		RaftStub stub;
		if (!stub.Init(ip, port)) {
			std::cerr << "ERROR: failed to connect to " << ip << ":" << port << std::endl;
			return 1;
		}

		ClientReply reply = stub.SendClientCommand(cmd);
		stub.Close();

		if (!reply.IsValid()) {
			std::cerr << "ERROR: invalid reply from server" << std::endl;
			return 1;
		}

		if (reply.GetStatus() == CLIENT_STATUS_NOT_LEADER &&
			!reply.GetLeaderIP().empty() && reply.GetLeaderPort() > 0) {
			ip = reply.GetLeaderIP();
			port = reply.GetLeaderPort();
			continue;
		}

		if (reply.GetStatus() == CLIENT_STATUS_OK) {
			if (op == "GET") {
				std::cout << reply.GetValue() << std::endl;
			} else {
				std::cout << "OK" << std::endl;
			}
			return 0;
		}

		if (reply.GetStatus() == CLIENT_STATUS_NOT_FOUND) {
			std::cout << "NOT_FOUND" << std::endl;
			return 0;
		}

		std::cerr << "ERROR: " << reply.GetMessage() << std::endl;
		return 1;
	}

	std::cerr << "ERROR: unable to reach leader after redirects" << std::endl;
	return 1;
}