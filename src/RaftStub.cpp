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

ClientReply RaftStub::SendClientCommand(ClientCommand cmd) {
	ClientReply reply;
	std::vector<char> req_buffer(cmd.Size());
	cmd.Marshal(req_buffer.data());
	if (!socket.Send(req_buffer.data(), cmd.Size(), 0)) {
		return reply;
	}

	std::vector<char> header(ClientReply::HeaderSize());
	if (!socket.Recv(header.data(), ClientReply::HeaderSize(), MSG_PEEK)) {
		return reply;
	}

	int total_size = ClientReply::ReadTotalSizeFromHeader(header.data());
	if (total_size <= 0) {
		return reply;
	}

	std::vector<char> reply_buffer(total_size);
	if (socket.Recv(reply_buffer.data(), total_size, 0)) {
		reply.Unmarshal(reply_buffer.data());
	}
	return reply;
}

void RaftStub::Close() {
	socket.Close();
}