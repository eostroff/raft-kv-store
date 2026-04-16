# Raft KV Store (C++)

A C++ implementation of the Raft consensus algorithm for a distributed, fault-tolerant key-value store.

Current codebase status: **Complete** – All core Raft features implemented and tested.

## What is implemented

- Raft node states: `FOLLOWER`, `CANDIDATE`, `LEADER`
- Randomized election timeout and leader election via `RequestVote` RPCs
- Leader heartbeats and log replication via `AppendEntries` RPCs
- Log conflict resolution and follower catch-up on rejoin
- Key-value state machine with `GET`, `PUT`, `DELETE` operations
- Commit index tracking and entry application to state machine
- Client command protocol with leader redirection for writes sent to followers
- Term update and demotion behavior when higher terms are discovered
- Multi-threaded design: listener thread, election timer thread, heartbeat thread, peer handler threads

## What is not implemented

- Durable persistence (all state is in-memory)
- Automated testing (tests are manual command sequences)

## Repository layout

```text
.
├── README.md
├── LICENSE
└── src/
    ├── Makefile                 # Compilation rules for server and client
    ├── ServerMain.cpp           # Server entrypoint; parses args and launches RaftNode
    ├── ClientMain.cpp           # Client entrypoint; sends PUT/GET/DELETE commands
    ├── RaftNode.{h,cpp}         # Raft state machine, election, heartbeat, and client handlers
    ├── LogManager.{h,cpp}       # Log structure and append/conflict-resolution logic
    ├── Messages.{h,cpp}         # RPC message types and serialization (RequestVote, AppendEntries, client commands)
    ├── RaftStub.{h,cpp}         # Client-side RPC stub for outbound requests
    ├── NodeStub.{h,cpp}         # Server-side RPC stub for received requests/replies
    ├── Socket.{h,cpp}           # Base TCP socket class
    ├── ListenSocket.{h,cpp}     # Server listening socket
    └── PeerClientSocket.{h,cpp} # Client connection to peer nodes
```

---

## Installation and Compilation on Khoury Linux Cluster

### Prerequisites

- SSH access to Khoury Linux cluster machines
- Port range 12346–12348 available between machines

### Step 1: Clone/Copy Project

Copy or clone this project to your home directory on any Khoury machine:

### Step 2: Navigate to src Directory

```bash
cd ~/raft-kv-store/src
```

### Step 3: Compile

```bash
make clean
make
```

This produces two executable binaries:
- `server` – Raft node process
- `client` – Client for issuing GET/PUT/DELETE commands

---

## Quick Start: Running a 3-Node Cluster on Khoury Linux

### Setup: Choose Three Machines and Get Their IPs

Open three separate terminal windows/SSH sessions:

Terminal 1 (Machine A):
```bash
hostname -i   # Record this IP as IP_A
cd ~/raft-kv-store/src
```

Terminal 2 (Machine B):
```bash
hostname -i   # Record this IP as IP_B
cd ~/raft-kv-store/src
```

Terminal 3 (Machine C):
```bash
hostname -i   # Record this IP as IP_C
cd ~/raft-kv-store/src
```

### Fresh Cluster Startup

Replace `IP_A`, `IP_B`, `IP_C` with the actual IPs from above.

**Machine A (Node 0, port 12346):**
```bash
./server 12346 0 2 1 IP_B 12347 2 IP_C 12348
```

**Machine B (Node 1, port 12347):**
```bash
./server 12347 1 2 0 IP_A 12346 2 IP_C 12348
```

**Machine C (Node 2, port 12348):**
```bash
./server 12348 2 2 0 IP_A 12346 1 IP_B 12347
```

### Verify Cluster is Running

Watch the server output in each terminal. You should see:
- `[Node X] Listening on port 12346/12347/12348`
- After ~0.5 seconds, one node prints: `[Node X] BECAME LEADER for term 1`
- Other nodes show vote messages and become followers

---

## Client Commands

From any machine in the cluster, run client commands. The client automatically redirects to the leader.

```bash
./client <server_ip> <server_port> GET <key>
./client <server_ip> <server_port> PUT <key> <value>
./client <server_ip> <server_port> DELETE <key>
```

Example (replace IPs/ports as needed):
```bash
./client IP_A 12346 PUT mykey myvalue
./client IP_B 12347 GET mykey
./client IP_C 12348 DELETE mykey
```

---

## Evaluation Tests

Run these tests from a fourth machine or terminal window (e.g., linux-074). Each test sends client commands and verifies behavior.

### Test 1: Leader Election and Re-election

1. Write a test key:
```bash
./client IP_A 12346 PUT test alive
./client IP_A 12346 GET test
# Expected: OK then alive
```

2. Identify the leader from the server logs (whichever terminal shows `BECAME LEADER`).

3. Kill the leader by pressing **Ctrl+C** in its server terminal.

4. Within ~1 second, watch the other server terminals. One should print: `[Node X] BECAME LEADER for term 2`

5. Verify the key is still accessible:
```bash
./client IP_A 12346 GET test  # May fail if A was the leader
./client IP_B 12347 GET test  # Should return: alive
./client IP_C 12348 GET test  # Should return: alive
```

**Expected result:** Two nodes respond with `alive`; killed node gives connection error.

---

### Test 2: Log Replication

1. Send 100 PUT operations:
```bash
for i in $(seq 1 100); do ./client IP_A 12346 PUT key$i val$i; done
```

2. Verify key1 on all nodes:
```bash
./client IP_A 12346 GET key1   # Expected: val1
./client IP_B 12347 GET key1   # Expected: val1
./client IP_C 12348 GET key1   # Expected: val1
```

3. Verify key50 on all nodes:
```bash
./client IP_A 12346 GET key50  # Expected: val50
./client IP_B 12347 GET key50  # Expected: val50
./client IP_C 12348 GET key50  # Expected: val50
```

4. Verify key100 on all nodes:
```bash
./client IP_A 12346 GET key100 # Expected: val100
./client IP_B 12347 GET key100 # Expected: val100
./client IP_C 12348 GET key100 # Expected: val100
```

**Expected result:** All 100 keys replicate to all three nodes; reads are consistent.

---

### Test 3: Log Conflict Resolution

1. Write an initial value:
```bash
./client IP_A 12346 PUT x before
```

2. Kill one follower (Ctrl+C in its server terminal; pick a non-leader).

3. Write more values to the leader while the follower is down:
```bash
./client IP_A 12346 PUT x after1
./client IP_A 12346 PUT x after2
./client IP_A 12346 PUT y newkey
```

4. Restart the killed follower using its exact original command:
```bash
./server 12348 2 2 0 IP_A 12346 1 IP_B 12347  # (if C was killed)
```

5. Wait ~2 seconds for heartbeats and catch-up.

6. Verify the restarted node has the latest values:
```bash
./client IP_A 12346 GET x  # Expected: after2
./client IP_B 12347 GET x  # Expected: after2
./client IP_C 12348 GET x  # Expected: after2

./client IP_A 12346 GET y  # Expected: newkey
./client IP_B 12347 GET y  # Expected: newkey
./client IP_C 12348 GET y  # Expected: newkey
```

**Expected result:** Restarted node catches up and converges to leader's log; all keys match across nodes.

---

### Test 4: Fault Tolerance

1. Write a test key:
```bash
./client IP_A 12346 PUT ft_key stillworks
./client IP_A 12346 GET ft_key
# Expected: OK then stillworks
```

2. Kill both follower nodes (Ctrl+C in both non-leader server terminals).

3. Attempt a write while majority is lost:
```bash
./client IP_A 12346 PUT ft_key shouldfail
# Expected: ERROR: commit timeout (or very delayed in some cases)
```

4. Verify the leader has NOT yet applied the uncommitted entry:
```bash
./client IP_A 12346 GET ft_key
# Expected: stillworks (old value; new entry not committed yet)
```

5. Restart both killed followers with their original commands:
```bash
./server 12347 1 2 0 IP_A 12346 2 IP_C 12348  # (if B was killed)
./server 12348 2 2 0 IP_A 12346 1 IP_B 12347  # (if C was killed)
```

6. Wait ~2 seconds for re-election, replication, and late commit.

7. Verify the timed-out entry eventually commits:
```bash
./client IP_A 12346 GET ft_key  # Expected: shouldfail
./client IP_B 12347 GET ft_key  # Expected: shouldfail
./client IP_C 12348 GET ft_key  # Expected: shouldfail
```

**Expected result:** With majority lost, writes block. After majority returns, pending entries eventually commit and are visible on all nodes.

---

### Test 5: Performance

1. Send 500 PUT operations and measure time:
```bash
time for i in $(seq 1 500); do ./client IP_A 12346 PUT perf$i x >/dev/null || break; done
```

2. From the output, note the `real` elapsed time (in seconds).

3. Calculate:
   - **Throughput** = 500 / real_seconds (ops/sec)
   - **Average latency** = (real_seconds × 1000) / 500 (milliseconds)

Example:
```
real    0m5.234s
Throughput: 500 / 5.234 = 95.5 ops/sec
Avg latency: (5.234 * 1000) / 500 = 10.5 ms
```

**Expected result:** Throughput typically 50–150 ops/sec on Khoury machines; average latency 7–20 ms per operation.

---

## Troubleshooting

### Build fails with "command not found: make"
- Ensure you are in the `src/` directory: `cd ~/raft-kv-store/src`

### "ERROR: failed to bind" or "Address already in use"
- The port is occupied. Either:
  - Wait 30 seconds and retry (TIME_WAIT socket state).
  - Use different port numbers (modify the startup commands).
- Verify ports 12346–12348 are reachable between machines: `telnet IP_B 12347`

### "ERROR: failed to connect: Connection refused"
- The target server has not started yet, or is on a different machine/port.
- Verify the IP and port in your client command.
- Confirm all three servers are running in their respective terminals.

### Cluster keeps re-electing (election churn)
- Confirm all three servers are running **simultaneously** in separate terminals.
- Verify each server has the **correct peer IP addresses** in its command.
- Network latency may cause temporary partitions; wait a few seconds.

### Client command hangs
- Press **Ctrl+C** to cancel.
- Ensure at least 2 of 3 servers are running (majority required).
- Try connecting to a different server IP/port.

---

## Notes

- **All state is in-memory.** Restarting a server means it restarts with empty logs.
- **No persistence:** The system recovers through Raft replication, not durable storage.
- **3-second client timeout:** Write operations wait max 3 seconds for commit confirmation before timing out.
- **Ports 12346–12348** are used in examples; you can adjust as needed (ensure no conflicts).
- **Leader redirection** is automatic—you can send any command to any node IP/port and it will forward or redirect as needed.

---

## Reproduction Checklist

To reproduce all results in your report:

- [ ] Compile on a Khoury machine: `cd src && make`
- [ ] Choose three distinct Khoury machines (e.g., linux-071, 072, 073).
- [ ] Get each machine's IP via `hostname -i`.
- [ ] Start all three servers in three separate SSH sessions with the correct IPs.
- [ ] From a fourth terminal/machine, run client commands as specified in each test.
- [ ] Record console output and observe expected behavior.
- [ ] For performance test, capture the `time` output and calculate throughput/latency.

All tests are deterministic and should pass consistently across multiple runs on Khoury Linux machines with the commands above.
