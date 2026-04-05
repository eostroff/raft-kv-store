#include "RaftStub.h"

RaftStub::RaftStub() {}

int RaftStub::Init(std::string ip, int port) {
	return socket.Init(ip, port);
}

RequestVoteReply RaftStub::SendRequestVote(RequestVote req) {
	RequestVoteReply reply;
	char buffer[64];
	req.Marshal(buffer);
	if (socket.Send(buffer, req.Size(), 0)) {
		if (socket.Recv(buffer, reply.Size(), 0)) {
			reply.Unmarshal(buffer);
		}
	}
	return reply;
}

AppendEntriesReply RaftStub::SendAppendEntries(AppendEntries ae) {
	AppendEntriesReply reply;
	char buffer[64];
	ae.Marshal(buffer);
	if (socket.Send(buffer, ae.Size(), 0)) {
		if (socket.Recv(buffer, reply.Size(), 0)) {
			reply.Unmarshal(buffer);
		}
	}
	return reply;
}

void RaftStub::Close() {
	socket.Close();
}