#include "RaftNode.h"

#include <iostream>
#include <algorithm>

// ============================================================
// Constructor / Destructor
// ============================================================

RaftNode::RaftNode(int id, int port, std::vector<PeerInfo> peers)
	: node_id(id), port(port), peers(peers),
	  current_term(0), voted_for(-1),
	  state(FOLLOWER), leader_id(-1),
	  commit_index(0), last_applied(0),
	  next_command_id(1),
	  running(false),
	  rng(std::random_device{}() + id) // seed with id for uniqueness
{
	ResetElectionTimer();
}

RaftNode::~RaftNode() {
	Stop();
}

// ============================================================
// Public interface
// ============================================================

void RaftNode::Start() {
	running = true;
	std::cout << "[Node " << node_id << "] Starting on port " << port << std::endl;

	listener_thread = std::thread(&RaftNode::ListenerThread, this);
	election_thread = std::thread(&RaftNode::ElectionTimerThread, this);

	// Wait for threads
	listener_thread.join();
	election_thread.join();
	if (heartbeat_thread.joinable()) {
		heartbeat_thread.join();
	}
	for (auto &t : peer_handler_threads) {
		if (t.joinable()) {
			t.join();
		}
	}
}

void RaftNode::Stop() {
	running = false;
	election_cv.notify_all();
}

// ============================================================
// Election timer
// ============================================================

void RaftNode::ResetElectionTimer() {
	last_heartbeat = std::chrono::steady_clock::now();
	election_timeout_ms = GetRandomTimeout();
}

int RaftNode::GetRandomTimeout() {
	std::uniform_int_distribution<int> dist(ELECTION_TIMEOUT_MIN, ELECTION_TIMEOUT_MAX);
	return dist(rng);
}

void RaftNode::ElectionTimerThread() {
	while (running) {
		std::unique_lock<std::mutex> lock(mu);
		// Sleep for a fraction of the timeout then check
		lock.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(50));

		lock.lock();
		if (!running) break;
		if (state == LEADER) {
			// Leaders don't run election timeouts
			lock.unlock();
			continue;
		}

		auto now = std::chrono::steady_clock::now();
		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			now - last_heartbeat).count();

		if (elapsed >= election_timeout_ms) {
			std::cout << "[Node " << node_id << "] Election timeout ("
				<< election_timeout_ms << "ms elapsed=" << elapsed
				<< "ms), starting election" << std::endl;
			lock.unlock();
			StartElection();
		}
	}
}

// ============================================================
// Listener thread - accepts incoming peer connections
// ============================================================

void RaftNode::ListenerThread() {
	ListenSocket server_socket;
	if (!server_socket.Init(port)) {
		std::cerr << "[Node " << node_id << "] Failed to init server socket on port "
			<< port << std::endl;
		running = false;
		return;
	}
	std::cout << "[Node " << node_id << "] Listening on port " << port << std::endl;

	while (running) {
		auto new_socket = server_socket.Accept();
		if (new_socket) {
			std::thread handler(&RaftNode::HandlePeerConnection, this, std::move(new_socket));
			peer_handler_threads.push_back(std::move(handler));
		}
	}
}

// ============================================================
// Handle an incoming peer connection (serves RPCs)
// ============================================================

void RaftNode::HandlePeerConnection(std::unique_ptr<ListenSocket> socket) {
	NodeStub stub;
	stub.Init(std::move(socket));

	while (running) {
		int rpc_type = stub.ReceiveRPCType();
		if (rpc_type < 0) {
			// Connection closed
			break;
		}

		if (rpc_type == RPC_REQUEST_VOTE) {
			RequestVote rv = stub.ReceiveRequestVote();
			if (rv.IsValid()) {
				HandleRequestVote(stub, rv);
			}
		} else if (rpc_type == RPC_APPEND_ENTRIES) {
			AppendEntries ae = stub.ReceiveAppendEntries();
			if (ae.IsValid()) {
				HandleAppendEntries(stub, ae);
			}
		} else if (rpc_type == RPC_CLIENT_COMMAND) {
			ClientCommand cmd = stub.ReceiveClientCommand();
			if (cmd.IsValid()) {
				HandleClientCommand(stub, cmd);
			}
		} else {
			std::cerr << "[Node " << node_id << "] Unknown RPC type: " << rpc_type << std::endl;
			break;
		}
	}
}

// ============================================================
// RPC Handlers
// ============================================================

void RaftNode::HandleRequestVote(NodeStub &stub, RequestVote &rv) {
	std::lock_guard<std::mutex> lock(mu);

	RequestVoteReply reply;
	int grant = 0;

	if (rv.GetTerm() > current_term) {
		// Discovered higher term, become follower
		current_term = rv.GetTerm();
		state = FOLLOWER;
		voted_for = -1;
		leader_id = -1;
	}

	if (rv.GetTerm() < current_term) {
		// Stale request
		grant = 0;
	} else if (voted_for == -1 || voted_for == rv.GetCandidateId()) {
		// Haven't voted yet this term (or already voted for this candidate)
		int my_last_term = log_manager.LastTerm();
		int my_last_index = log_manager.LastIndex();
		bool candidate_up_to_date =
			(rv.GetLastLogTerm() > my_last_term) ||
			(rv.GetLastLogTerm() == my_last_term && rv.GetLastLogIndex() >= my_last_index);

		if (candidate_up_to_date) {
			grant = 1;
			voted_for = rv.GetCandidateId();
			ResetElectionTimer();
			std::cout << "[Node " << node_id << "] Voted for node " << rv.GetCandidateId()
				<< " in term " << current_term << std::endl;
		}
	}

	reply.SetReply(current_term, grant, node_id);
	stub.SendRequestVoteReply(reply);
}

void RaftNode::HandleAppendEntries(NodeStub &stub, AppendEntries &ae) {
	std::lock_guard<std::mutex> lock(mu);

	AppendEntriesReply reply;
	int success = 0;

	if (ae.GetTerm() < current_term) {
		success = 0;
	} else {
		if (ae.GetTerm() > current_term) {
			BecomeFollower(ae.GetTerm());
		}
		state = FOLLOWER;
		leader_id = ae.GetLeaderId();
		ResetElectionTimer();

		success = log_manager.AppendFromLeader(
			ae.GetPrevLogIndex(), ae.GetPrevLogTerm(), ae.GetEntries()) ? 1 : 0;

		if (success && ae.GetLeaderCommit() > commit_index) {
			commit_index = std::min(ae.GetLeaderCommit(), log_manager.LastIndex());
			ApplyCommittedEntries();
		}
	}

	reply.SetReply(current_term, success, node_id);
	stub.SendAppendEntriesReply(reply);
}

void RaftNode::HandleClientCommand(NodeStub &stub, ClientCommand &cmd) {
	ClientReply reply;

	if (cmd.GetOp() == CLIENT_OP_GET) {
		std::lock_guard<std::mutex> lock(mu);
		auto it = kv_store.find(cmd.GetKey());
		if (it == kv_store.end()) {
			reply.SetReply(CLIENT_STATUS_NOT_FOUND, 0, "", "", "key not found");
		} else {
			reply.SetReply(CLIENT_STATUS_OK, 0, "", it->second, "ok");
		}
		stub.SendClientReply(reply);
		return;
	}

	int cmd_type = (cmd.GetOp() == CLIENT_OP_PUT) ? CMD_PUT : CMD_DELETE;
	int entry_index = -1;
	int start_term = 0;

	{
		std::lock_guard<std::mutex> lock(mu);
		if (state != LEADER) {
			PeerInfo leader = FindPeerById(leader_id);
			reply.SetReply(CLIENT_STATUS_NOT_LEADER, leader.port, leader.ip, "", "not leader");
			stub.SendClientReply(reply);
			return;
		}

		start_term = current_term;
		entry_index = log_manager.Append(current_term, cmd_type, cmd.GetKey(), cmd.GetValue());
	}

	for (int waited_ms = 0; waited_ms < 3000 && running; waited_ms += 20) {
		{
			std::lock_guard<std::mutex> lock(mu);
			if (state != LEADER || current_term != start_term) {
				PeerInfo leader = FindPeerById(leader_id);
				reply.SetReply(CLIENT_STATUS_NOT_LEADER, leader.port, leader.ip, "", "leadership changed");
				stub.SendClientReply(reply);
				return;
			}

			if (commit_index >= entry_index) {
				ApplyCommittedEntries();
				reply.SetReply(CLIENT_STATUS_OK, 0, "", "", "ok");
				stub.SendClientReply(reply);
				return;
			}
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
	}

	reply.SetReply(CLIENT_STATUS_ERROR, 0, "", "", "commit timeout");
	stub.SendClientReply(reply);
}

// ============================================================
// Election Logic
// ============================================================

void RaftNode::StartElection() {
	std::unique_lock<std::mutex> lock(mu);

	// Transition to candidate
	state = CANDIDATE;
	current_term++;
	voted_for = node_id;
	int election_term = current_term;
	int last_log_index = log_manager.LastIndex();
	int last_log_term = log_manager.LastTerm();
	ResetElectionTimer();

	std::cout << "[Node " << node_id << "] Starting election for term " << election_term << std::endl;

	int votes_received = 1; // Vote for self
	int total_nodes = (int)peers.size() + 1;
	int majority = total_nodes / 2 + 1;

	lock.unlock();

	// Send RequestVote RPCs to all peers in parallel
	std::vector<std::thread> vote_threads;

	for (auto &peer : peers) {
		vote_threads.emplace_back([&, peer]() {
			RaftStub stub;
			if (!stub.Init(peer.ip, peer.port)) {
				return;
			}

			RequestVote rv;
			rv.SetRequest(election_term, node_id, last_log_index, last_log_term);

			RequestVoteReply reply = stub.SendRequestVote(rv);
			stub.Close();

			if (!reply.IsValid()) return;

			std::lock_guard<std::mutex> nlock(mu);

			// Check if we're still a candidate in the same term
			if (state != CANDIDATE || current_term != election_term) return;

			if (reply.GetTerm() > current_term) {
				// Discovered higher term
				BecomeFollower(reply.GetTerm());
				return;
			}

			if (reply.GetVoteGranted()) {
				votes_received++;
				std::cout << "[Node " << node_id << "] Received vote from node "
					<< reply.GetVoterId() << " (" << votes_received
					<< "/" << majority << " needed)" << std::endl;

				if (votes_received >= majority && state == CANDIDATE) {
					BecomeLeader();
				}
			}
		});
	}

	for (auto &t : vote_threads) {
		t.join();
	}
}

void RaftNode::BecomeFollower(int term) {
	// Caller must hold mu
	std::cout << "[Node " << node_id << "] Becoming follower in term " << term << std::endl;
	current_term = term;
	state = FOLLOWER;
	voted_for = -1;
	leader_id = -1;
	next_index.clear();
	match_index.clear();
	ResetElectionTimer();
}

void RaftNode::BecomeLeader() {
	// Caller must hold mu
	state = LEADER;
	leader_id = node_id;
	int last_index = log_manager.LastIndex();
	next_index.assign(peers.size(), last_index + 1);
	match_index.assign(peers.size(), 0);

	int noop_index = log_manager.Append(current_term, CMD_NOOP);
	std::cout << "[Node " << node_id << "] Appended leader no-op at index "
		<< noop_index << " term " << current_term << std::endl;

	std::cout << "========================================" << std::endl;
	std::cout << "[Node " << node_id << "] BECAME LEADER for term " << current_term << std::endl;
	std::cout << "========================================" << std::endl;

	// Start heartbeat thread
	if (heartbeat_thread.joinable()) {
		heartbeat_thread.join();
	}
	heartbeat_thread = std::thread(&RaftNode::HeartbeatThread, this);
}

void RaftNode::MaybeAdvanceCommitIndex() {
	int total_nodes = (int)peers.size() + 1;
	int majority = total_nodes / 2 + 1;
	for (int idx = log_manager.LastIndex(); idx > commit_index; --idx) {
		if (log_manager.TermAt(idx) != current_term) {
			continue;
		}
		int replicated = 1;
		for (int m : match_index) {
			if (m >= idx) {
				replicated++;
			}
		}
		if (replicated >= majority) {
			commit_index = idx;
			ApplyCommittedEntries();
			return;
		}
	}
}

void RaftNode::ApplyCommittedEntries() {
	while (last_applied < commit_index) {
		last_applied++;
		LogEntry e;
		if (!log_manager.GetEntry(last_applied, e)) {
			break;
		}

		if (e.command == CMD_PUT) {
			kv_store[e.key] = e.value;
		} else if (e.command == CMD_DELETE) {
			kv_store.erase(e.key);
		}
	}
}

// ============================================================
// Heartbeat Thread (leader only)
// ============================================================

void RaftNode::HeartbeatThread() {
	std::cout << "[Node " << node_id << "] Heartbeat thread started" << std::endl;

	while (running) {
		{
			std::lock_guard<std::mutex> lock(mu);
			if (state != LEADER) {
				std::cout << "[Node " << node_id << "] No longer leader, stopping heartbeats" << std::endl;
				return;
			}
		}

		// Send heartbeats to all peers
		int my_term;
		{
			std::lock_guard<std::mutex> lock(mu);
			my_term = current_term;
		}

		std::vector<std::thread> hb_threads;
		for (size_t i = 0; i < peers.size(); ++i) {
			PeerInfo peer = peers[i];
			hb_threads.emplace_back([&, i, peer, my_term]() {
				int peer_next_index;
				int prev_log_index;
				int prev_log_term;
				int leader_commit_snapshot;
				std::vector<LogEntry> entries_to_send;

				{
					std::lock_guard<std::mutex> lock(mu);
					if (state != LEADER || current_term != my_term) {
						return;
					}

					peer_next_index = next_index[i];
					prev_log_index = peer_next_index - 1;
					prev_log_term = log_manager.TermAt(prev_log_index);
					leader_commit_snapshot = commit_index;
					entries_to_send = log_manager.EntriesFrom(peer_next_index);
				}

				RaftStub stub;
				if (!stub.Init(peer.ip, peer.port)) {
					return;
				}

				AppendEntries ae;
				ae.SetEntries(my_term, node_id, prev_log_index, prev_log_term,
					leader_commit_snapshot, entries_to_send);

				AppendEntriesReply reply = stub.SendAppendEntries(ae);
				stub.Close();

				if (!reply.IsValid()) {
					return;
				}

				std::lock_guard<std::mutex> lock(mu);
				if (state != LEADER || current_term != my_term) {
					return;
				}

				if (reply.GetTerm() > current_term) {
					BecomeFollower(reply.GetTerm());
					return;
				}

				if (reply.GetSuccess()) {
					int replicated_to = prev_log_index + (int)entries_to_send.size();
					match_index[i] = replicated_to;
					next_index[i] = replicated_to + 1;
					MaybeAdvanceCommitIndex();
				} else if (next_index[i] > 1) {
					next_index[i]--;
				}
			});
		}

		for (auto &t : hb_threads) {
			t.join();
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
	}
}

// ============================================================
// Utility
// ============================================================

std::string RaftNode::StateStr() {
	switch (state) {
		case FOLLOWER:  return "FOLLOWER";
		case CANDIDATE: return "CANDIDATE";
		case LEADER:    return "LEADER";
		default:        return "UNKNOWN";
	}
}

PeerInfo RaftNode::FindPeerById(int id) const {
	if (id == node_id) {
		PeerInfo me;
		me.id = node_id;
		me.ip = "127.0.0.1";
		me.port = port;
		return me;
	}
	for (size_t i = 0; i < peers.size(); ++i) {
		if (peers[i].id == id) {
			return peers[i];
		}
	}
	PeerInfo empty;
	empty.id = -1;
	empty.port = 0;
	return empty;
}