#include "NodeStub.h"

#include <vector>

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
	std::vector<char> buffer(rv.Size());
	if (socket->Recv(buffer.data(), rv.Size(), 0)) {
		rv.Unmarshal(buffer.data());
	}
	return rv;
}

AppendEntries NodeStub::ReceiveAppendEntries() {
	AppendEntries ae;
	std::vector<char> header(AppendEntries::HeaderSize());
	if (!socket->Recv(header.data(), AppendEntries::HeaderSize(), MSG_PEEK)) {
		return ae;
	}

	int total_size = AppendEntries::ReadTotalSizeFromHeader(header.data());
	if (total_size <= 0) {
		return ae;
	}

	std::vector<char> buffer(total_size);
	if (socket->Recv(buffer.data(), total_size, 0)) {
		ae.Unmarshal(buffer.data());
	}
	return ae;
}

int NodeStub::SendRequestVoteReply(RequestVoteReply reply) {
	std::vector<char> buffer(reply.Size());
	reply.Marshal(buffer.data());
	return socket->Send(buffer.data(), reply.Size(), 0);
}

int NodeStub::SendAppendEntriesReply(AppendEntriesReply reply) {
	std::vector<char> buffer(reply.Size());
	reply.Marshal(buffer.data());
	return socket->Send(buffer.data(), reply.Size(), 0);
}

int NodeStub::SendRequestVote(RequestVote req) {
	std::vector<char> buffer(req.Size());
	req.Marshal(buffer.data());
	return socket->Send(buffer.data(), req.Size(), 0);
}

int NodeStub::SendAppendEntries(AppendEntries ae) {
	std::vector<char> buffer(ae.Size());
	ae.Marshal(buffer.data());
	return socket->Send(buffer.data(), ae.Size(), 0);
}

RequestVoteReply NodeStub::ReceiveRequestVoteReply() {
	RequestVoteReply reply;
	std::vector<char> buffer(reply.Size());
	if (socket->Recv(buffer.data(), reply.Size(), 0)) {
		reply.Unmarshal(buffer.data());
	}
	return reply;
}

AppendEntriesReply NodeStub::ReceiveAppendEntriesReply() {
	AppendEntriesReply reply;
	std::vector<char> buffer(reply.Size());
	if (socket->Recv(buffer.data(), reply.Size(), 0)) {
		reply.Unmarshal(buffer.data());
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