# Raft KV Store (C++)

A C++ implementation of the core Raft consensus election/heartbeat flow, structured as a distributed key-value store project.

Current codebase status: **Progress Report 2 (PR2)** level functionality.

## What is implemented

- Raft node states: `FOLLOWER`, `CANDIDATE`, `LEADER`
- Randomized election timeout and leader election
- `RequestVote` and `RequestVoteReply` RPCs
- Leader heartbeats via `AppendEntries`
- Log replication with conflict resolution and follower catch-up
- In-memory key-value state machine application on commit
- Client command protocol for `GET`, `PUT`, and `DELETE`
- Leader redirection replies for client writes sent to followers
- Term update/demotion behavior when higher term is discovered
- Manual peer configuration via command-line arguments
- Multi-threaded listener, election timer, and heartbeat loops

## What is not implemented yet

- Durable persistence across process restarts (state is in-memory)
- Automated integration/performance test harness

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

## Server startup format

Each server is started with explicit peer information:

```text
./server <my_port> <node_id> <num_peers> [<peer_id> <peer_ip> <peer_port> ...]
```

This is intended for manually launching nodes on different machines (for example via SSH), without any shared config file.

`num_peers` must match the number of peer triples provided.

## Running a 3-node cluster (manual, multi-machine friendly)

Example with three machines:

- Node 0 on `10.0.0.10`, port `8000`
- Node 1 on `10.0.0.11`, port `8001`
- Node 2 on `10.0.0.12`, port `8002`

Run on machine A:

```bash
cd src
./server 8000 0 2 1 10.0.0.11 8001 2 10.0.0.12 8002
```

Run on machine B:

```bash
cd src
./server 8001 1 2 0 10.0.0.10 8000 2 10.0.0.12 8002
```

Run on machine C:

```bash
cd src
./server 8002 2 2 0 10.0.0.10 8000 1 10.0.0.11 8001
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

- `ERROR: my_port must be a positive integer`: check local port argument.
- `ERROR: num_peers must be non-negative`: check peer count argument.
- `ERROR: argument error(s) for peer list`: verify that `num_peers` matches provided triples.
- `ERROR: Peer id X matches local node_id`: remove local id from peer list.
- `ERROR: Peer port must be a positive integer`: check each peer triple.
- Frequent election churn: confirm all nodes are running concurrently.
- Frequent election churn: confirm all declared peer ports are reachable and not occupied.
- Frequent election churn: verify every node uses a consistent peer map.

## Development notes

- Build artifacts (`*.o`, `server`, `client`) are generated under `src/`.
- Use `make clean` to remove generated files.

## Next milestones (suggested)

1. Add Raft log structure and AppendEntries log replication.
2. Add commit/apply pipeline into a key-value state machine.
3. Implement real client protocol (`PUT`, `GET`, `DELETE`) with leader forwarding.
4. Add integration tests for election stability and failover.
