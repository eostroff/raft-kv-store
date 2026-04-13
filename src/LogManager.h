#ifndef __LOG_MANAGER_H__
#define __LOG_MANAGER_H__

#include <string>
#include <vector>

enum CommandType {
	CMD_NOOP = 0,
	CMD_PUT = 1,
	CMD_DELETE = 2
};

struct LogEntry {
	int term;
	int command;
	std::string key;
	std::string value;
};

class LogManager {
private:
	std::vector<LogEntry> entries;

public:
	LogManager();

	int LastIndex() const;
	int LastTerm() const;
	int TermAt(int index) const;
	bool HasMatchingEntry(int index, int term) const;

	int Append(int term, int command, const std::string &key = "",
		const std::string &value = "");
	std::vector<LogEntry> EntriesFrom(int start_index) const;
	bool GetEntry(int index, LogEntry &entry) const;

	// Raft conflict resolution:
	// 1) validate prev_log_index/term
	// 2) delete conflicting suffix
	// 3) append new entries
	bool AppendFromLeader(int prev_log_index, int prev_log_term,
		const std::vector<LogEntry> &new_entries);
};

#endif // end of #ifndef __LOG_MANAGER_H__