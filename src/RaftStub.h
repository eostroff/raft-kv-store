#ifndef __CLIENT_STUB_H__
#define __CLIENT_STUB_H__

#include <string>
#include "PeerClientSocket.h"
#include "Messages.h"

class RaftStub {
private:
	PeerClientSocket socket;
public:
	RaftStub();
	int Init(std::string ip, int port);

	// Send RPCs and receive replies
	RequestVoteReply SendRequestVote(RequestVote req);
	AppendEntriesReply SendAppendEntries(AppendEntries ae);
	ClientReply SendClientCommand(ClientCommand cmd);

	void Close();
};

#endif // end of #ifndef __CLIENT_STUB_H__