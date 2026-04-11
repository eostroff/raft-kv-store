#include "RaftStub.h"

#include <vector>

RaftStub::RaftStub() {}

int RaftStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);
}

RequestVoteReply RaftStub::SendRequestVote(RequestVote req) {
	RequestVoteReply reply;
	std::vector<char> req_buffer(req.Size());
	req.Marshal(req_buffer.data());
	if (socket.Send(req_buffer.data(), req.Size(), 0)) {
		std::vector<char> reply_buffer(reply.Size());
		if (socket.Recv(reply_buffer.data(), reply.Size(), 0)) {
			reply.Unmarshal(reply_buffer.data());
		}
	}
	return reply;
}

AppendEntriesReply RaftStub::SendAppendEntries(AppendEntries ae) {
	AppendEntriesReply reply;
	std::vector<char> req_buffer(ae.Size());
	ae.Marshal(req_buffer.data());
	if (socket.Send(req_buffer.data(), ae.Size(), 0)) {
		std::vector<char> reply_buffer(reply.Size());
		if (socket.Recv(reply_buffer.data(), reply.Size(), 0)) {
			reply.Unmarshal(reply_buffer.data());
		}
	}
	return reply;
}

void RaftStub::Close() {
	socket.Close();
}