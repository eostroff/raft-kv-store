#ifndef __RAFT_NODE_H__
#define __RAFT_NODE_H__

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "Messages.h"
#include "LogManager.h"
#include "ListenSocket.h"
#include "NodeStub.h"
#include "RaftStub.h"

enum NodeState {
	FOLLOWER = 0,
	CANDIDATE = 1,
	LEADER = 2
};

struct PeerInfo {
	int id;
	std::string ip;
	int port;
};

class RaftNode {
private:
	// ---- Identity ----
	int node_id;
	int port;
	std::vector<PeerInfo> peers;

	// ---- Persistent Raft state ----
	int current_term;
	int voted_for;       // -1 if none
	LogManager log_manager;

	// ---- Volatile state ----
	NodeState state;
	int leader_id;       // -1 if unknown
	int commit_index;
	int last_applied;

	// ---- Leader state (re-initialized on election) ----
	std::vector<int> next_index;
	std::vector<int> match_index;
	int next_command_id;
	std::unordered_map<std::string, std::string> kv_store;

	// ---- Synchronization ----
	std::mutex mu;
	std::condition_variable election_cv;
	std::atomic<bool> running;

	// ---- Election timer ----
	std::chrono::steady_clock::time_point last_heartbeat;
	int election_timeout_ms;  // randomized per reset
	std::mt19937 rng;

	// ---- Threads ----
	std::thread listener_thread;
	std::thread election_thread;
	std::thread heartbeat_thread;
	std::vector<std::thread> peer_handler_threads;

	// ---- Election timeout range (ms) ----
	enum {
		ELECTION_TIMEOUT_MIN = 300,
		ELECTION_TIMEOUT_MAX = 500,
		HEARTBEAT_INTERVAL_MS = 100
	};

	// ---- Internal methods ----
	void ResetElectionTimer();
	int GetRandomTimeout();

	// Thread bodies
	void ListenerThread();
	void HandlePeerConnection(std::unique_ptr<ListenSocket> socket);
	void ElectionTimerThread();
	void HeartbeatThread();

	// RPC handlers (called from HandlePeerConnection)
	void HandleRequestVote(NodeStub &stub, RequestVote &rv);
	void HandleAppendEntries(NodeStub &stub, AppendEntries &ae);
	void HandleClientCommand(NodeStub &stub, ClientCommand &cmd);

	// Election logic
	void StartElection();
	void BecomeFollower(int term);
	void BecomeLeader();
	void MaybeAdvanceCommitIndex();
	void ApplyCommittedEntries();
	PeerInfo FindPeerById(int id) const;

	// Helper: get state as string for logging
	std::string StateStr();

public:
	RaftNode(int id, int port, std::vector<PeerInfo> peers);
	~RaftNode();

	void Start();
	void Stop();
};

#endif // end of #ifndef __RAFT_NODE_H__