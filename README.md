# Raft KV Store (C++)

A C++ implementation of the core Raft consensus election/heartbeat flow, structured as a distributed key-value store project.

Current codebase status: **Progress Report 1 (PR1)** level functionality.

## What is implemented

- Raft node states: `FOLLOWER`, `CANDIDATE`, `LEADER`
- Randomized election timeout and leader election
- `RequestVote` and `RequestVoteReply` RPCs
- Leader heartbeats via `AppendEntries` (empty entries)
- Term update/demotion behavior when higher term is discovered
- Multi-node configuration file parsing
- Multi-threaded listener, election timer, and heartbeat loops

## What is not implemented yet

- Replicated log entries
- Commit index / apply-to-state-machine logic
- Key-value `GET/PUT/DELETE` operations
- Full client interaction with leader redirection and retries

The `client` binary currently prints a placeholder message and does not issue KV commands.

## Repository layout

```text
.
├── README.md
├── LICENSE
└── src/
    ├── Makefile
    ├── ServerMain.cpp          # server entrypoint
    ├── ClientMain.cpp          # client entrypoint (placeholder)
    ├── RaftNode.*              # Raft state machine and threads
    ├── Messages.*              # RPC message serialization
    ├── RaftStub.*              # outgoing RPC client wrapper
    ├── NodeStub.*              # incoming RPC server-side wrapper
    ├── Socket.*                # socket abstraction
    ├── ListenSocket.*          # listening/accepted socket abstraction
    └── PeerClientSocket.*      # peer connection abstraction
```

## Build

Requirements:

- Linux/macOS environment with `g++`
- `make`
- POSIX threads support (`-pthread`)

Build commands:

```bash
cd src
make clean
make
```

This produces two binaries in `src/`:

- `server`
- `client`

Debug build:

```bash
cd src
make clean
make debug
```

## Cluster configuration file

Each server reads a shared config file with one node per line:

```text
node_id ip port
```

Example (`cluster.conf`):

```text
0 127.0.0.1 8000
1 127.0.0.1 8001
2 127.0.0.1 8002
```

Notes:

- Lines beginning with `#` are ignored.
- Blank lines are ignored.
- Each node must have a unique `node_id` and `port`.

## Running a 3-node local cluster

From the `src/` directory, open 3 terminals:

Terminal 1:

```bash
./server 0 ../cluster.conf
```

Terminal 2:

```bash
./server 1 ../cluster.conf
```

Terminal 3:

```bash
./server 2 ../cluster.conf
```

You should see:

- Nodes starting and listening on their configured ports
- One node timing out and starting an election
- Vote messages and majority acquisition
- A node printing that it became leader
- Ongoing heartbeat behavior while leadership is maintained

## Running the client (current placeholder)

```bash
cd src
./client 127.0.0.1 8000
```

Current behavior:

- Prints connection target
- Reports that full KV client support will be added in PR2

## Raft behavior implemented in this phase

- Followers reset election timeout when valid leader heartbeat is received.
- On timeout, a follower becomes candidate, increments term, and votes for itself.
- Candidate sends `RequestVote` RPCs to all peers in parallel.
- On majority vote, candidate becomes leader and starts periodic heartbeats.
- If any node sees a higher term in an RPC reply/request, it steps down to follower.

## Troubleshooting

- `ERROR: Could not open config file`: verify path passed to `server`.
- `Node id X not found in config file`: ensure the node id exists in the config.
- Frequent election churn: confirm all nodes are running concurrently.
- Frequent election churn: confirm all ports in config are reachable and not occupied.
- Frequent election churn: verify every node uses the same config content.

## Development notes

- Build artifacts (`*.o`, `server`, `client`) are generated under `src/`.
- Use `make clean` to remove generated files.

## Next milestones (suggested)

1. Add Raft log structure and AppendEntries log replication.
2. Add commit/apply pipeline into a key-value state machine.
3. Implement real client protocol (`PUT`, `GET`, `DELETE`) with leader forwarding.
4. Add integration tests for election stability and failover.
