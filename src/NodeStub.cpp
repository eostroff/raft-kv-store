#include "NodeStub.h"

NodeStub::NodeStub() {}

void NodeStub::Init(std::unique_ptr<ListenSocket> socket) {
	this->socket = std::move(socket);
}

int NodeStub::ReceiveRPCType() {
	char buffer[4];
	if (!socket->Recv(buffer, 4, MSG_PEEK)) {
		return -1;
	}
	return PeekRPCType(buffer);
}

RequestVote NodeStub::ReceiveRequestVote() {
	RequestVote rv;
	char buffer[64];
	if (socket->Recv(buffer, rv.Size(), 0)) {
		rv.Unmarshal(buffer);
	}
	return rv;
}

AppendEntries NodeStub::ReceiveAppendEntries() {
	AppendEntries ae;
	char buffer[64];
	if (socket->Recv(buffer, ae.Size(), 0)) {
		ae.Unmarshal(buffer);
	}
	return ae;
}

int NodeStub::SendRequestVoteReply(RequestVoteReply reply) {
	char buffer[64];
	reply.Marshal(buffer);
	return socket->Send(buffer, reply.Size(), 0);
}

int NodeStub::SendAppendEntriesReply(AppendEntriesReply reply) {
	char buffer[64];
	reply.Marshal(buffer);
	return socket->Send(buffer, reply.Size(), 0);
}

int NodeStub::SendRequestVote(RequestVote req) {
	char buffer[64];
	req.Marshal(buffer);
	return socket->Send(buffer, req.Size(), 0);
}

int NodeStub::SendAppendEntries(AppendEntries ae) {
	char buffer[64];
	ae.Marshal(buffer);
	return socket->Send(buffer, ae.Size(), 0);
}

RequestVoteReply NodeStub::ReceiveRequestVoteReply() {
	RequestVoteReply reply;
	char buffer[64];
	if (socket->Recv(buffer, reply.Size(), 0)) {
		reply.Unmarshal(buffer);
	}
	return reply;
}

AppendEntriesReply NodeStub::ReceiveAppendEntriesReply() {
	AppendEntriesReply reply;
	char buffer[64];
	if (socket->Recv(buffer, reply.Size(), 0)) {
		reply.Unmarshal(buffer);
	}
	return reply;
}

bool NodeStub::IsValid() {
	return (socket != nullptr);
}

void NodeStub::Close() {
	if (socket) {
		socket->Close();
	}
}