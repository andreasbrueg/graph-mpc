# PPGA Tests

The tests run on a specific, hardscripted graph, and run assertions to check correctness of the
outputs, and also run assertions to check that the amount of communication matches the protocol
specification. Note that the assertions require to compile the code to debug mode:
```sh
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j8
```

For each test, execute the different commands in different terminals, one for each server/client.

### BFS 3 Parallel

Testing [algorithms/bfs_3_parallel.h](../algorithms/bfs_3_parallel.h).

```sh
./test_bfs_3_parallel --localhost --pid 0
./test_bfs_3_parallel --localhost --pid 1
./test_bfs_3_parallel --localhost --pid 2
```

### BFS Multiple Sources

Testing [algorithms/bfs](../algorithms/bfs.h), using multiple BFS sources.

```sh
./test_bfs_multiple_sources --localhost --pid 0
./test_bfs_multiple_sources --localhost --pid 1
./test_bfs_multiple_sources --localhost --pid 2
```

### BFS

Testing [algorithms/bfs](../algorithms/bfs.h), with a single BFS source.

```sh
./test_bfs --localhost --pid 0
./test_bfs --localhost --pid 1
./test_bfs --localhost --pid 2
```

### Permutation

Running some local permutation tests.
```sh
./test_perm
```

### Pi_K

Testing [algorithms/pi_k](../algorithms/pi_k.h).

```sh
./test_pi_k --localhost --pid 0
./test_pi_k --localhost --pid 1
./test_pi_k --localhost --pid 2
```

### Pi_M with clients

Testing [algorithms/pi_m](../algorithms/pi_m.h) with clients providing inputs and receiving outputs.

```sh
./test_pi_m_clients --localhost --pid 0
./test_pi_m_clients --localhost --pid 1
./test_pi_m_clients --localhost --pid 2
./test_pi_m_clients_client --localhost --isclient --cid 0
./test_pi_m_clients_client --localhost --isclient --cid 1
```

### Pi_M

Testing [algorithms/pi_m](../algorithms/pi_m.h).

```sh
./test_pi_m --localhost --pid 0
./test_pi_m --localhost --pid 1
./test_pi_m --localhost --pid 2
```

### Pi_R

Testing [algorithms/pi_r](../algorithms/pi_r.h).

```sh
./test_pi_r --localhost --pid 0
./test_pi_r --localhost --pid 1
./test_pi_r --localhost --pid 2
```

