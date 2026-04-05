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
		// Log up-to-date check (simplified for progress report 1 - no log yet)
		grant = 1;
		voted_for = rv.GetCandidateId();
		ResetElectionTimer();
		std::cout << "[Node " << node_id << "] Voted for node " << rv.GetCandidateId()
			<< " in term " << current_term << std::endl;
	}

	reply.SetReply(current_term, grant, node_id);
	stub.SendRequestVoteReply(reply);
}

void RaftNode::HandleAppendEntries(NodeStub &stub, AppendEntries &ae) {
	std::lock_guard<std::mutex> lock(mu);

	AppendEntriesReply reply;
	int success = 0;

	if (ae.GetTerm() >= current_term) {
		// Valid leader heartbeat
		if (ae.GetTerm() > current_term) {
			current_term = ae.GetTerm();
			voted_for = -1;
		}
		state = FOLLOWER;
		leader_id = ae.GetLeaderId();
		ResetElectionTimer();
		success = 1;

		// Log replication will be handled in progress report 2
	}
	// If ae.GetTerm() < current_term, reject (success stays 0)

	reply.SetReply(current_term, success, node_id);
	stub.SendAppendEntriesReply(reply);
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
	ResetElectionTimer();

	std::cout << "[Node " << node_id << "] Starting election for term " << election_term << std::endl;

	int votes_received = 1; // Vote for self
	int total_nodes = (int)peers.size() + 1;
	int majority = total_nodes / 2 + 1;

	lock.unlock();

	// Send RequestVote RPCs to all peers in parallel
	std::vector<std::thread> vote_threads;
	std::mutex vote_mu;

	for (auto &peer : peers) {
		vote_threads.emplace_back([&, peer]() {
			RaftStub stub;
			if (!stub.Init(peer.ip, peer.port)) {
				std::cerr << "[Node " << node_id << "] Failed to connect to peer "
					<< peer.id << " at " << peer.ip << ":" << peer.port << std::endl;
				return;
			}

			RequestVote rv;
			rv.SetRequest(election_term, node_id, 0, 0); // last_log_index/term = 0 for now

			RequestVoteReply reply = stub.SendRequestVote(rv);
			stub.Close();

			if (!reply.IsValid()) return;

			std::lock_guard<std::mutex> vlock(vote_mu);
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
	ResetElectionTimer();
}

void RaftNode::BecomeLeader() {
	// Caller must hold mu
	state = LEADER;
	leader_id = node_id;
	std::cout << "========================================" << std::endl;
	std::cout << "[Node " << node_id << "] BECAME LEADER for term " << current_term << std::endl;
	std::cout << "========================================" << std::endl;

	// Start heartbeat thread (detached - will check running flag)
	if (heartbeat_thread.joinable()) {
		// Previous heartbeat thread from a prior leadership - it should have
		// stopped when we lost leadership. We detach to avoid blocking.
		heartbeat_thread.detach();
	}
	heartbeat_thread = std::thread(&RaftNode::HeartbeatThread, this);
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
		for (auto &peer : peers) {
			hb_threads.emplace_back([&, peer, my_term]() {
				RaftStub stub;
				if (!stub.Init(peer.ip, peer.port)) {
					return;
				}

				AppendEntries ae;
				ae.SetEntries(my_term, node_id, 0, 0, 0, 0); // heartbeat: 0 entries

				AppendEntriesReply reply = stub.SendAppendEntries(ae);
				stub.Close();

				if (reply.IsValid() && reply.GetTerm() > my_term) {
					std::lock_guard<std::mutex> lock(mu);
					if (reply.GetTerm() > current_term) {
						BecomeFollower(reply.GetTerm());
					}
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