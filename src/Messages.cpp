#include <iostream>
#include "Messages.h"

namespace {
int WriteStringField(char *buffer, int offset, const std::string &s) {
	int len = (int)s.size();
	int net_len = htonl(len);
	memcpy(buffer + offset, &net_len, sizeof(net_len));
	offset += (int)sizeof(net_len);
	if (len > 0) {
		memcpy(buffer + offset, s.data(), len);
		offset += len;
	}
	return offset;
}

int ReadStringField(char *buffer, int offset, std::string &out) {
	int net_len;
	memcpy(&net_len, buffer + offset, sizeof(net_len));
	offset += (int)sizeof(net_len);
	int len = ntohl(net_len);
	if (len < 0) {
		out.clear();
		return -1;
	}
	out.assign(buffer + offset, buffer + offset + len);
	offset += len;
	return offset;
}
}

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
	return (int)(8 * sizeof(int));
}

int AppendEntries::ReadTotalSizeFromHeader(char *header) {
	int net_total_size;
	memcpy(&net_total_size, header + (int)(7 * sizeof(int)), sizeof(net_total_size));
	int total = ntohl(net_total_size);
	if (total < HeaderSize()) {
		return -1;
	}
	return total;
}

int AppendEntries::Size() {
	int total = HeaderSize();
	for (int i = 0; i < entry_count; ++i) {
		total += (int)(2 * sizeof(int));
		total += (int)sizeof(int) + (int)entries[i].key.size();
		total += (int)sizeof(int) + (int)entries[i].value.size();
	}
	return total;
}

void AppendEntries::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_term = htonl(term);
	int net_leader_id = htonl(leader_id);
	int net_prev_log_index = htonl(prev_log_index);
	int net_prev_log_term = htonl(prev_log_term);
	int net_leader_commit = htonl(leader_commit);
	int net_entry_count = htonl(entry_count);
	int net_total_size = htonl(Size());
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
	memcpy(buffer + offset, &net_total_size, sizeof(net_total_size));
	offset += sizeof(net_total_size);

	for (int i = 0; i < entry_count; ++i) {
		int net_entry_term = htonl(entries[i].term);
		int net_entry_command = htonl(entries[i].command);
		memcpy(buffer + offset, &net_entry_term, sizeof(net_entry_term));
		offset += sizeof(net_entry_term);
		memcpy(buffer + offset, &net_entry_command, sizeof(net_entry_command));
		offset += sizeof(net_entry_command);
		offset = WriteStringField(buffer, offset, entries[i].key);
		offset = WriteStringField(buffer, offset, entries[i].value);
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
	int net_total_size;
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
	memcpy(&net_total_size, buffer + offset, sizeof(net_total_size));
	offset += sizeof(net_total_size);

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
		offset = ReadStringField(buffer, offset, e.key);
		offset = ReadStringField(buffer, offset, e.value);
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

// ---- ClientCommand ----

ClientCommand::ClientCommand() {
	rpc_type = RPC_CLIENT_COMMAND;
	op = -1;
}

void ClientCommand::SetCommand(int op, const std::string &key, const std::string &value) {
	this->op = op;
	this->key = key;
	this->value = value;
}

int ClientCommand::HeaderSize() {
	return (int)(5 * sizeof(int));
}

int ClientCommand::ReadTotalSizeFromHeader(char *header) {
	int net_total_size;
	memcpy(&net_total_size, header + (int)(4 * sizeof(int)), sizeof(net_total_size));
	int total = ntohl(net_total_size);
	if (total < HeaderSize()) {
		return -1;
	}
	return total;
}

int ClientCommand::Size() {
	return HeaderSize() + (int)key.size() + (int)value.size();
}

void ClientCommand::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_op = htonl(op);
	int net_key_len = htonl((int)key.size());
	int net_value_len = htonl((int)value.size());
	int net_total_size = htonl(Size());
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += (int)sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_op, sizeof(net_op));
	offset += (int)sizeof(net_op);
	memcpy(buffer + offset, &net_key_len, sizeof(net_key_len));
	offset += (int)sizeof(net_key_len);
	memcpy(buffer + offset, &net_value_len, sizeof(net_value_len));
	offset += (int)sizeof(net_value_len);
	memcpy(buffer + offset, &net_total_size, sizeof(net_total_size));
	offset += (int)sizeof(net_total_size);
	if (!key.empty()) {
		memcpy(buffer + offset, key.data(), key.size());
		offset += (int)key.size();
	}
	if (!value.empty()) {
		memcpy(buffer + offset, value.data(), value.size());
	}
}

void ClientCommand::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_op;
	int net_key_len;
	int net_value_len;
	int net_total_size;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += (int)sizeof(net_rpc_type);
	memcpy(&net_op, buffer + offset, sizeof(net_op));
	offset += (int)sizeof(net_op);
	memcpy(&net_key_len, buffer + offset, sizeof(net_key_len));
	offset += (int)sizeof(net_key_len);
	memcpy(&net_value_len, buffer + offset, sizeof(net_value_len));
	offset += (int)sizeof(net_value_len);
	memcpy(&net_total_size, buffer + offset, sizeof(net_total_size));
	offset += (int)sizeof(net_total_size);

	rpc_type = ntohl(net_rpc_type);
	op = ntohl(net_op);
	int key_len = ntohl(net_key_len);
	int value_len = ntohl(net_value_len);
	key.assign(buffer + offset, buffer + offset + key_len);
	offset += key_len;
	value.assign(buffer + offset, buffer + offset + value_len);
}

bool ClientCommand::IsValid() {
	return rpc_type == RPC_CLIENT_COMMAND &&
		(op == CLIENT_OP_GET || op == CLIENT_OP_PUT || op == CLIENT_OP_DELETE);
}

void ClientCommand::Print() {
	std::cout << "[ClientCommand] op=" << op << " key=" << key << std::endl;
}

// ---- ClientReply ----

ClientReply::ClientReply() {
	rpc_type = RPC_CLIENT_REPLY;
	status = CLIENT_STATUS_ERROR;
	leader_port = 0;
}

void ClientReply::SetReply(int status, int leader_port, const std::string &leader_ip,
	const std::string &value, const std::string &message) {
	this->status = status;
	this->leader_port = leader_port;
	this->leader_ip = leader_ip;
	this->value = value;
	this->message = message;
}

int ClientReply::HeaderSize() {
	return (int)(7 * sizeof(int));
}

int ClientReply::ReadTotalSizeFromHeader(char *header) {
	int net_total_size;
	memcpy(&net_total_size, header + (int)(6 * sizeof(int)), sizeof(net_total_size));
	int total = ntohl(net_total_size);
	if (total < HeaderSize()) {
		return -1;
	}
	return total;
}

int ClientReply::Size() {
	return HeaderSize() + (int)leader_ip.size() + (int)value.size() + (int)message.size();
}

void ClientReply::Marshal(char *buffer) {
	int net_rpc_type = htonl(rpc_type);
	int net_status = htonl(status);
	int net_leader_port = htonl(leader_port);
	int net_ip_len = htonl((int)leader_ip.size());
	int net_value_len = htonl((int)value.size());
	int net_message_len = htonl((int)message.size());
	int net_total_size = htonl(Size());
	int offset = 0;
	memcpy(buffer + offset, &net_rpc_type, sizeof(net_rpc_type));
	offset += (int)sizeof(net_rpc_type);
	memcpy(buffer + offset, &net_status, sizeof(net_status));
	offset += (int)sizeof(net_status);
	memcpy(buffer + offset, &net_leader_port, sizeof(net_leader_port));
	offset += (int)sizeof(net_leader_port);
	memcpy(buffer + offset, &net_ip_len, sizeof(net_ip_len));
	offset += (int)sizeof(net_ip_len);
	memcpy(buffer + offset, &net_value_len, sizeof(net_value_len));
	offset += (int)sizeof(net_value_len);
	memcpy(buffer + offset, &net_message_len, sizeof(net_message_len));
	offset += (int)sizeof(net_message_len);
	memcpy(buffer + offset, &net_total_size, sizeof(net_total_size));
	offset += (int)sizeof(net_total_size);
	if (!leader_ip.empty()) {
		memcpy(buffer + offset, leader_ip.data(), leader_ip.size());
		offset += (int)leader_ip.size();
	}
	if (!value.empty()) {
		memcpy(buffer + offset, value.data(), value.size());
		offset += (int)value.size();
	}
	if (!message.empty()) {
		memcpy(buffer + offset, message.data(), message.size());
	}
}

void ClientReply::Unmarshal(char *buffer) {
	int net_rpc_type;
	int net_status;
	int net_leader_port;
	int net_ip_len;
	int net_value_len;
	int net_message_len;
	int net_total_size;
	int offset = 0;
	memcpy(&net_rpc_type, buffer + offset, sizeof(net_rpc_type));
	offset += (int)sizeof(net_rpc_type);
	memcpy(&net_status, buffer + offset, sizeof(net_status));
	offset += (int)sizeof(net_status);
	memcpy(&net_leader_port, buffer + offset, sizeof(net_leader_port));
	offset += (int)sizeof(net_leader_port);
	memcpy(&net_ip_len, buffer + offset, sizeof(net_ip_len));
	offset += (int)sizeof(net_ip_len);
	memcpy(&net_value_len, buffer + offset, sizeof(net_value_len));
	offset += (int)sizeof(net_value_len);
	memcpy(&net_message_len, buffer + offset, sizeof(net_message_len));
	offset += (int)sizeof(net_message_len);
	memcpy(&net_total_size, buffer + offset, sizeof(net_total_size));
	offset += (int)sizeof(net_total_size);

	rpc_type = ntohl(net_rpc_type);
	status = ntohl(net_status);
	leader_port = ntohl(net_leader_port);
	int ip_len = ntohl(net_ip_len);
	int value_len = ntohl(net_value_len);
	int message_len = ntohl(net_message_len);

	leader_ip.assign(buffer + offset, buffer + offset + ip_len);
	offset += ip_len;
	value.assign(buffer + offset, buffer + offset + value_len);
	offset += value_len;
	message.assign(buffer + offset, buffer + offset + message_len);
}

bool ClientReply::IsValid() {
	return rpc_type == RPC_CLIENT_REPLY;
}

void ClientReply::Print() {
	std::cout << "[ClientReply] status=" << status << " message=" << message << std::endl;
}

// ---- Utility ----

int PeekRPCType(char *buffer) {
	int net_type;
	memcpy(&net_type, buffer, sizeof(net_type));
	return ntohl(net_type);
}