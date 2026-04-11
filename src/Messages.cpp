#include <iostream>
#include "Messages.h"

// ---- RequestVote ----

RequestVote::RequestVote() {
	rpc_type = RPC_REQUEST_VOTE;
	term = -1;
	candidate_id = -1;
	last_log_index = -1;
	last_log_term = -1;
}

void RequestVote::SetRequest(int t, int cid, int lli, int llt) {
	term = t;
	candidate_id = cid;
	last_log_index = lli;
	last_log_term = llt;
}

int RequestVote::Size() {
	return sizeof(rpc_type) + sizeof(term) + sizeof(candidate_id)
		+ sizeof(last_log_index) + sizeof(last_log_term);
}

void RequestVote::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_term = htonl(term);
	int net_candidate_id = htonl(candidate_id);
	int net_last_log_index = htonl(last_log_index);
	int net_last_log_term = htonl(last_log_term);
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_term, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(buffer + offset, &net_candidate_id, sizeof(net_candidate_id));
	offset += sizeof(net_candidate_id);
	memcpy(buffer + offset, &net_last_log_index, sizeof(net_last_log_index));
	offset += sizeof(net_last_log_index);
	memcpy(buffer + offset, &net_last_log_term, sizeof(net_last_log_term));
}

void RequestVote::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_term;
	int net_candidate_id;
	int net_last_log_index;
	int net_last_log_term;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(&net_term, buffer + offset, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(&net_candidate_id, buffer + offset, sizeof(net_candidate_id));
	offset += sizeof(net_candidate_id);
	memcpy(&net_last_log_index, buffer + offset, sizeof(net_last_log_index));
	offset += sizeof(net_last_log_index);
	memcpy(&net_last_log_term, buffer + offset, sizeof(net_last_log_term));

	rpc_type = ntohl(net_rpc_type);
	term = ntohl(net_term);
	candidate_id = ntohl(net_candidate_id);
	last_log_index = ntohl(net_last_log_index);
	last_log_term = ntohl(net_last_log_term);
}

bool RequestVote::IsValid() {
	return (term != -1);
}

void RequestVote::Print() {
	std::cout << "[RequestVote] term=" << term
		<< " candidate=" << candidate_id
		<< " lastLogIdx=" << last_log_index
		<< " lastLogTerm=" << last_log_term << std::endl;
}

// ---- RequestVoteReply ----

RequestVoteReply::RequestVoteReply() {
	rpc_type = RPC_REQUEST_VOTE_REPLY;
	term = -1;
	vote_granted = 0;
	voter_id = -1;
}

void RequestVoteReply::SetReply(int t, int granted, int vid) {
	term = t;
	vote_granted = granted;
	voter_id = vid;
}

int RequestVoteReply::Size() {
	return sizeof(rpc_type) + sizeof(term) + sizeof(vote_granted) + sizeof(voter_id);
}

void RequestVoteReply::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_term = htonl(term);
	int net_vote_granted = htonl(vote_granted);
	int net_voter_id = htonl(voter_id);
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_term, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(buffer + offset, &net_vote_granted, sizeof(net_vote_granted));
	offset += sizeof(net_vote_granted);
	memcpy(buffer + offset, &net_voter_id, sizeof(net_voter_id));
}

void RequestVoteReply::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_term;
	int net_vote_granted;
	int net_voter_id;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(&net_term, buffer + offset, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(&net_vote_granted, buffer + offset, sizeof(net_vote_granted));
	offset += sizeof(net_vote_granted);
	memcpy(&net_voter_id, buffer + offset, sizeof(net_voter_id));

	rpc_type = ntohl(net_rpc_type);
	term = ntohl(net_term);
	vote_granted = ntohl(net_vote_granted);
	voter_id = ntohl(net_voter_id);
}

bool RequestVoteReply::IsValid() {
	return (term != -1);
}

void RequestVoteReply::Print() {
	std::cout << "[RequestVoteReply] term=" << term
		<< " granted=" << vote_granted
		<< " voter=" << voter_id << std::endl;
}

// ---- AppendEntries ----

AppendEntries::AppendEntries() {
	rpc_type = RPC_APPEND_ENTRIES;
	term = -1;
	leader_id = -1;
	prev_log_index = -1;
	prev_log_term = -1;
	leader_commit = -1;
	entry_count = 0;
}

void AppendEntries::SetEntries(int t, int lid, int pli, int plt, int lc,
	const std::vector<LogEntry> &new_entries) {
	term = t;
	leader_id = lid;
	prev_log_index = pli;
	prev_log_term = plt;
	leader_commit = lc;
	entries = new_entries;
	entry_count = (int)entries.size();
}

int AppendEntries::HeaderSize() {
	return (int)(7 * sizeof(int));
}

int AppendEntries::ReadTotalSizeFromHeader(char *header) {
	int net_entry_count;
	int offset = (int)(6 * sizeof(int));
	memcpy(&net_entry_count, header + offset, sizeof(net_entry_count));
	int parsed_entry_count = ntohl(net_entry_count);
	if (parsed_entry_count < 0) {
		return -1;
	}
	return HeaderSize() + parsed_entry_count * (int)(2 * sizeof(int));
}

int AppendEntries::Size() {
	return HeaderSize() + entry_count * (int)(2 * sizeof(int));
}

void AppendEntries::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_term = htonl(term);
	int net_leader_id = htonl(leader_id);
	int net_prev_log_index = htonl(prev_log_index);
	int net_prev_log_term = htonl(prev_log_term);
	int net_leader_commit = htonl(leader_commit);
	int net_entry_count = htonl(entry_count);
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_term, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(buffer + offset, &net_leader_id, sizeof(net_leader_id));
	offset += sizeof(net_leader_id);
	memcpy(buffer + offset, &net_prev_log_index, sizeof(net_prev_log_index));
	offset += sizeof(net_prev_log_index);
	memcpy(buffer + offset, &net_prev_log_term, sizeof(net_prev_log_term));
	offset += sizeof(net_prev_log_term);
	memcpy(buffer + offset, &net_leader_commit, sizeof(net_leader_commit));
	offset += sizeof(net_leader_commit);
	memcpy(buffer + offset, &net_entry_count, sizeof(net_entry_count));
	offset += sizeof(net_entry_count);

	for (int i = 0; i < entry_count; ++i) {
		int net_entry_term = htonl(entries[i].term);
		int net_entry_command = htonl(entries[i].command);
		memcpy(buffer + offset, &net_entry_term, sizeof(net_entry_term));
		offset += sizeof(net_entry_term);
		memcpy(buffer + offset, &net_entry_command, sizeof(net_entry_command));
		offset += sizeof(net_entry_command);
	}
}

void AppendEntries::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_term;
	int net_leader_id;
	int net_prev_log_index;
	int net_prev_log_term;
	int net_leader_commit;
	int net_entry_count;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(&net_term, buffer + offset, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(&net_leader_id, buffer + offset, sizeof(net_leader_id));
	offset += sizeof(net_leader_id);
	memcpy(&net_prev_log_index, buffer + offset, sizeof(net_prev_log_index));
	offset += sizeof(net_prev_log_index);
	memcpy(&net_prev_log_term, buffer + offset, sizeof(net_prev_log_term));
	offset += sizeof(net_prev_log_term);
	memcpy(&net_leader_commit, buffer + offset, sizeof(net_leader_commit));
	offset += sizeof(net_leader_commit);
	memcpy(&net_entry_count, buffer + offset, sizeof(net_entry_count));
	offset += sizeof(net_entry_count);

	rpc_type = ntohl(net_rpc_type);
	term = ntohl(net_term);
	leader_id = ntohl(net_leader_id);
	prev_log_index = ntohl(net_prev_log_index);
	prev_log_term = ntohl(net_prev_log_term);
	leader_commit = ntohl(net_leader_commit);
	entry_count = ntohl(net_entry_count);

	entries.clear();
	for (int i = 0; i < entry_count; ++i) {
		int net_entry_term;
		int net_entry_command;
		memcpy(&net_entry_term, buffer + offset, sizeof(net_entry_term));
		offset += sizeof(net_entry_term);
		memcpy(&net_entry_command, buffer + offset, sizeof(net_entry_command));
		offset += sizeof(net_entry_command);
		LogEntry e;
		e.term = ntohl(net_entry_term);
		e.command = ntohl(net_entry_command);
		entries.push_back(e);
	}
}

bool AppendEntries::IsValid() {
	return (term != -1);
}

void AppendEntries::Print() {
	std::cout << "[AppendEntries] term=" << term
		<< " leader=" << leader_id
		<< " prevLogIdx=" << prev_log_index
		<< " prevLogTerm=" << prev_log_term
		<< " leaderCommit=" << leader_commit
		<< " entries=" << entry_count << std::endl;
}

// ---- AppendEntriesReply ----

AppendEntriesReply::AppendEntriesReply() {
	rpc_type = RPC_APPEND_ENTRIES_REPLY;
	term = -1;
	success = 0;
	node_id = -1;
}

void AppendEntriesReply::SetReply(int t, int s, int nid) {
	term = t;
	success = s;
	node_id = nid;
}

int AppendEntriesReply::Size() {
	return sizeof(rpc_type) + sizeof(term) + sizeof(success) + sizeof(node_id);
}

void AppendEntriesReply::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_term = htonl(term);
	int net_success = htonl(success);
	int net_node_id = htonl(node_id);
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_term, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(buffer + offset, &net_success, sizeof(net_success));
	offset += sizeof(net_success);
	memcpy(buffer + offset, &net_node_id, sizeof(net_node_id));
}

void AppendEntriesReply::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_term;
	int net_success;
	int net_node_id;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += sizeof(net_rpc_type);
	memcpy(&net_term, buffer + offset, sizeof(net_term));
	offset += sizeof(net_term);
	memcpy(&net_success, buffer + offset, sizeof(net_success));
	offset += sizeof(net_success);
	memcpy(&net_node_id, buffer + offset, sizeof(net_node_id));

	rpc_type = ntohl(net_rpc_type);
	term = ntohl(net_term);
	success = ntohl(net_success);
	node_id = ntohl(net_node_id);
}

bool AppendEntriesReply::IsValid() {
	return (term != -1);
}

void AppendEntriesReply::Print() {
	std::cout << "[AppendEntriesReply] term=" << term
		<< " success=" << success
		<< " node=" << node_id << std::endl;
}

// ---- Utility ----

int PeekRPCType(char *buffer) {
	int net_type;
	memcpy(&net_type, buffer, sizeof(net_type));
	return ntohl(net_type);
}