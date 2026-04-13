#ifndef __NODE_STUB_H__
#define __NODE_STUB_H__

#include <memory>
#include "ListenSocket.h"
#include "Messages.h"

class NodeStub {
private:
	std::unique_ptr<ListenSocket> socket;
public:
	NodeStub();
	void Init(std::unique_ptr<ListenSocket> socket);

	// Receive the RPC type tag (first 4 bytes) to determine message type
	int ReceiveRPCType();

	// Receive specific messages (call after peeking the type)
	RequestVote ReceiveRequestVote();
	AppendEntries ReceiveAppendEntries();
	ClientCommand ReceiveClientCommand();

	// Send replies
	int SendRequestVoteReply(RequestVoteReply reply);
	int SendAppendEntriesReply(AppendEntriesReply reply);
	int SendClientReply(ClientReply reply);

	// Send requests (used by leader/candidate through peer connections)
	int SendRequestVote(RequestVote req);
	int SendAppendEntries(AppendEntries ae);

	// Receive replies
	RequestVoteReply ReceiveRequestVoteReply();
	AppendEntriesReply ReceiveAppendEntriesReply();
	ClientReply ReceiveClientReply();

	bool IsValid();
	void Close();
};

#endif // end of #ifndef __NODE_STUB_H__