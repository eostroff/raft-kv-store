#ifndef __MESSAGES_H__
#define __MESSAGES_H__

#include <string>
#include <cstring>
#include <arpa/inet.h>

// RPC type identifiers
enum RPCType {
	RPC_REQUEST_VOTE = 1,
	RPC_REQUEST_VOTE_REPLY = 2,
	RPC_APPEND_ENTRIES = 3,
	RPC_APPEND_ENTRIES_REPLY = 4
};

// Sent by candidates to request votes during election
class RequestVote {
private:
	int rpc_type;       // RPC_REQUEST_VOTE
	int term;           // candidate's term
	int candidate_id;   // candidate requesting vote
	int last_log_index; // index of candidate's last log entry
	int last_log_term;  // term of candidate's last log entry

public:
	RequestVote();
	void SetRequest(int t, int cid, int lli, int llt);

	int GetRPCType()      { return rpc_type; }
	int GetTerm()         { return term; }
	int GetCandidateId()  { return candidate_id; }
	int GetLastLogIndex() { return last_log_index; }
	int GetLastLogTerm()  { return last_log_term; }

	int Size();
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	bool IsValid();
	void Print();
};

// Reply to RequestVote
class RequestVoteReply {
private:
	int rpc_type;     // RPC_REQUEST_VOTE_REPLY
	int term;         // current term, for candidate to update itself
	int vote_granted; // 1 if candidate received vote, 0 otherwise
	int voter_id;     // id of the node that voted

public:
	RequestVoteReply();
	void SetReply(int t, int granted, int vid);

	int GetRPCType()     { return rpc_type; }
	int GetTerm()        { return term; }
	int GetVoteGranted() { return vote_granted; }
	int GetVoterId()     { return voter_id; }

	int Size();
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	bool IsValid();
	void Print();
};

// Sent by leader to replicate log entries and as heartbeat
class AppendEntries {
private:
	int rpc_type;        // RPC_APPEND_ENTRIES
	int term;            // leader's term
	int leader_id;       // so follower can redirect clients
	int prev_log_index;  // index of log entry immediately preceding new ones
	int prev_log_term;   // term of prev_log_index entry
	int leader_commit;   // leader's commit index
	int entry_count;     // number of entries (0 for heartbeat)
	// For progress report 1, we only send heartbeats (entry_count = 0)
	// Log entries will be added in progress report 2

public:
	AppendEntries();
	void SetEntries(int t, int lid, int pli, int plt, int lc, int ec);

	int GetRPCType()       { return rpc_type; }
	int GetTerm()          { return term; }
	int GetLeaderId()      { return leader_id; }
	int GetPrevLogIndex()  { return prev_log_index; }
	int GetPrevLogTerm()   { return prev_log_term; }
	int GetLeaderCommit()  { return leader_commit; }
	int GetEntryCount()    { return entry_count; }

	int Size();
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	bool IsValid();
	void Print();
};

// Reply to AppendEntries
class AppendEntriesReply {
private:
	int rpc_type;  // RPC_APPEND_ENTRIES_REPLY
	int term;      // current term, for leader to update itself
	int success;   // 1 if follower contained entry matching prev_log_index/term
	int node_id;   // id of the replying node

public:
	AppendEntriesReply();
	void SetReply(int t, int s, int nid);

	int GetRPCType() { return rpc_type; }
	int GetTerm()    { return term; }
	int GetSuccess() { return success; }
	int GetNodeId()  { return node_id; }

	int Size();
	void Marshal(char *buffer);
	void Unmarshal(char *buffer);
	bool IsValid();
	void Print();
};

// Utility: read just the RPC type from a buffer (first 4 bytes)
int PeekRPCType(char *buffer);

#endif // #ifndef __MESSAGES_H__