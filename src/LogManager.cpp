#include "LogManager.h"

LogManager::LogManager() {}

int LogManager::LastIndex() const {
	return (int)entries.size();
}

int LogManager::LastTerm() const {
	if (entries.empty()) {
		return 0;
	}
	return entries.back().term;
}

int LogManager::TermAt(int index) const {
	if (index == 0) {
		return 0;
	}
	if (index < 0 || index > (int)entries.size()) {
		return -1;
	}
	return entries[index - 1].term;
}

bool LogManager::HasMatchingEntry(int index, int term) const {
	if (index == 0) {
		return term == 0;
	}
	if (index < 0 || index > (int)entries.size()) {
		return false;
	}
	return entries[index - 1].term == term;
}

int LogManager::Append(int term, int command, const std::string &key,
	const std::string &value) {
	entries.push_back({term, command, key, value});
	return (int)entries.size();
}

std::vector<LogEntry> LogManager::EntriesFrom(int start_index) const {
	std::vector<LogEntry> out;
	if (start_index <= 0 || start_index > (int)entries.size() + 1) {
		return out;
	}
	for (int i = start_index - 1; i < (int)entries.size(); ++i) {
		out.push_back(entries[i]);
	}
	return out;
}

bool LogManager::GetEntry(int index, LogEntry &entry) const {
	if (index <= 0 || index > (int)entries.size()) {
		return false;
	}
	entry = entries[index - 1];
	return true;
}

bool LogManager::AppendFromLeader(int prev_log_index, int prev_log_term,
	const std::vector<LogEntry> &new_entries) {
	if (!HasMatchingEntry(prev_log_index, prev_log_term)) {
		return false;
	}

	int local_index = prev_log_index + 1;
	int incoming_offset = 0;

	while (incoming_offset < (int)new_entries.size() && local_index <= (int)entries.size()) {
		if (entries[local_index - 1].term != new_entries[incoming_offset].term) {
			entries.resize(local_index - 1);
			break;
		}
		local_index++;
		incoming_offset++;
	}

	for (int i = incoming_offset; i < (int)new_entries.size(); ++i) {
		entries.push_back(new_entries[i]);
	}

	return true;
}