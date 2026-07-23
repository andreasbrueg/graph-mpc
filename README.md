# PPGA Graph-MPC: Privacy-Preserving Graph Analysis using the Prepare/Propagate/Gather/Apply Paradigm

PPGA is a framework to run custom graph analysis algorithms based on message-passing in secure
multiparty computation (MPC) to keep the graph topology private.
In message-passing algorithms, nodes hold some state and then the algorithm runs multiple iterations
where each:
- **P**repares messages for each node, based on the node's state,
- **P**ropagates these messages from nodes to all their outgoing edges,
- **G**athers for each node an update which is the sum over all incoming messages, and
- **A**pplies the update to the node state, computing the next node state from the prior state and update.

PPGA lets multiple clients contribute their private parts of a bigger graph, and the PPGA core then
runs an algorithm from a list of pre-defined algorithms or a custom user-defined algorithm on the
graph in MPC, so that the clients learn the outputs for their nodes, but the compute servers learn
nothing but the graph dimensions for each client's input about the input data.
The PPGA core utilizes three compute servers (one of which acts as a dealer of correlated randomness).
It runs the [MultiCent protocol](https://eprint.iacr.org/2025/652) as its MPC backend, providing
security if the three servers follow the protocol specification and do not collude.

PPGA also provides a practical interface to define new custom graph algorithms, utilizing the PPGA
(Prepare/Propagate/Gather/Apply) paradigm and a library of pre-defined settings and functions.

> [!NOTE]
> This code is in an experimental stage; it should not be used in any production environment.

## Getting Started: A Quick Demo on Using PPGA

We first provide a simple guide to get used to how PPGA works, for simplicity running all servers
and clients on the same machine within a Docker container. Throughout our guide, we recommend using
multiple terminals to observe the behavior of all servers and clients at the same time.

First, we set up and run the Docker container as follows:
```sh
sudo docker buildx build -t PPGA .
sudo docker run -it -v $(pwd):$(pwd) -w $(pwd) --cap-add=NET_ADMIN PPGA
```
For our example, it is best to use five separate terminals. In each new terminal, you can simply
access the running container as follows:
```sh
TODO
```
Proceed by compiling the code as follows in one of the terminals:
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```
Perfect, everything is now ready to run graph algorithms!

Suppose you are Alice and want to outsource computation on your private graph which looks as follows:
```
n 1 --> n 2 --> n 3 --> n 4
 |       ^
 v       |
n 5 <-- n 6 <-- n 7 <-- n 8
```
This graph is specified in the file ```subgraph_0.data``` which looks as follows:
```
n 1 0
n 2 0
n 3 0
n 4 0
n 5 0
n 6 0
n 7 1
n 8 0
e 1 2
e 2 3
e 3 4
e 8 7
e 7 6
e 6 5
e 1 5
e 6 2
```
Each entry **n [id] [state]** represents a node with a unique id and some initial state.
If the state shall be zero or the algorithm ignores user state inputs, you can also leave out this
second number. An entry **e [v] [w]** represents a directed edge from the node with id **v** to node
**w**. For undirected edges, simply add directed edges into both directions. PPGA even supports
multigraphs, so you can have multiple copies of the same edge; just the nodes need to be unique.

Alice wants the PPGA compute servers to run a BFS of depth 3 on the graph to see which nodes are
reachable from **node 7** in at most three hops. She simply encodes a node being reached by state
**1** and an unreached node by state **0**, so for the input, she only sets the starting node **n 7**
to **1**.

Next, start the servers as follows. Run each of the following commands in one separate terminal, one
for each of the three servers:
```sh
./run_server --algorithm 0 --depth 3 --localhost --pid 0 --clients 1
./run_server --algorithm 0 --depth 3 --localhost --pid 1 --clients 1
./run_server --algorithm 0 --depth 3 --localhost --pid 2 --clients 1
```
This runs **algorithm 0** (which here corresponds to BFS) with a depth of **3**, communicating over
the localhost interface and expecting a single client. The **pid** simply selects the role of each
of the three servers (server **2** is the dealer).

Now, Alice provides the inputs (use yet another of your five terminals for that) by running
```sh
./run_client --cid 0 --localhost --clients 1
```
Alice has client id **0**, also connects via localhost, and again, the total number of participating
clients is 1.

In the outputs for Alice, you can now see the used graph and parameters. The servers receive the
graph size, then run the algorithm, and finally reveal the result to Alice without ever having seen
the graph themselves. Alice receives
```
node 1 state 0
node 2 state 1
node 3 state 1
node 4 state 0
node 5 state 1
node 6 state 1
node 7 state 1
node 8 state 0
```
She learns that nodes 2, 3, 5, 6, and 7 have been reached within at most 3 hops from node 7, which
she can verify looking at the graph as shown before.

Of course, Alice could simply run the BFS herself, but running computation on a graph held by a single
party is not the goal of PPGA! Let's take her friend Bob into consideration. Bob also has a private
graph. He also knows that Alice's graph has nodes 5, 6, 7, and 8. (In a practical application,
assume that Alice and Bob have partially overlapping graphs and in advance identify overlapping nodes,
using some privacy-preserving approach like private set intersection (PSI). For more pointers, we
refer to Section 4.1 of the [MultiCent paper](https://eprint.iacr.org/2025/652).)
Bob's graph is as follows, where nodes 5 to 8 are not *his nodes*, but he knows that Alice has them
and how his nodes connect to them, without knowing about any connections between nodes 5 to 8:
```
(n 5)   (n 6)   (n 7)   (n 8)
  ^       |      | ^      ^
  |       v      v |      |
 n 9 --> n10 --> n11 --> n12
  |       ^               |
  v       |               v
 n13 <-- n14 <-- n15 <-- n16
```
The graph is defined in file ```subgraph_1.data```. Alice and Bob now want to glue their graphs
together and run computations on the bigger graph, *without showing their graphs to each other*.
First, run the three servers again, changing the last argument so that they now expect two clients:
```sh
./run_server --algorithm 0 --depth 3 --localhost --pid 0 --clients 2
./run_server --algorithm 0 --depth 3 --localhost --pid 1 --clients 2
./run_server --algorithm 0 --depth 3 --localhost --pid 2 --clients 2
```
Next, we run Alice, also changing the number of clients:
```sh
./run_client --cid 0 --localhost --clients 2
```
Now, in the fifth terminal, we run Bob:
```sh
 ./run_client --cid 1 --localhost --clients 2 --offset 16
```
Note that besides Bob's client id, this also specifies an offset. Here, it matches the graph size
provided by Alice (not the graph content), i.e., the number of nodes plus the number of edges.
Technically speaking, Alice by default uses offset zero, meaning that the provides the first graph
entries (nodes or edges), whereas Bob provides the 16th, 17th, etc. entry.

In the outputs, we now first observe that Alice provides her graph of size 16 (with 8 nodes) and
Bob provides a graph of size 22 (with 8 nodes), so the servers each receive secret shares corresponding
to 38 entries including 16 nodes.
Bob now also learns that, from whatever starting point Alice selected, his nodes 10, 11, 12, and 16
are reachable in three hops. To better visualize that, here is the complete graph, noting that this
will never be disclosed to any server, nor to Alice and Bob:
```
n 1 --> n 2 --> n 3 --> n 4
 |       ^
 v       |
n 5 <-- n 6 <-- n 7 <-- n 8
 ^       |      | ^      ^
 |       v      v |      |
n 9 --> n10 --> n11 --> n12
 |       ^               |
 v       |               v
n13 <-- n14 <-- n15 <-- n16
```
Alice now also sees that in addition to the prior experiment, node 8 has been reached, which was only
possible using a path that first went over to Bob's subgraph and then back to Alice's subgraph.

#### Next Steps:

To experiment around, you can, e.g., try the other algorithms. All available algorithms can be listed
using ```./run_server --help```. Also, you may make changes to the input files of Alice and Bob.
Note that cmake copies them from the main directory into the build directory, so you need to directly
edit those in the build directory to change the graph inputs.

For a more detailed description of the different options, and instructions on how to implement your
own custom graph algorithm, check out the rest of the documentation in this readme file!


## Installing PPGA

PPGA has the following external dependencies, which are already included in our Dockerfile, but need
to manually be installed if PPGA is not used from a docker container:

* [GMP](https://gmplib.org)
* [Boost](https://www.boost.org)
* [OpenSSL](https://github.com/openssl/openssl)
* [EMP Tool](https://github.com/emp-toolkit/emp-tool)
* [nlohmann_json](https://github.com/nlohmann/json)
* [OpenMP](https://www.openmp.org)

GMP, Boost, OpenSSL, nlohmann_json, and OpenMP are available from standard package repositories on
many distributions. For EMP tool, it usually suffices to install it at a freely chosen path as follows:
```sh
git clone https://github.com/emp-toolkit/emp-tool.git
cd emp-tool
git checkout 8052d95
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 .
make -j8
make install
```

Instead, for local tests in a Docker container, the environment can simply be set up via
```sh
sudo docker buildx build -t PPGA .
sudo docker run -it -v $(pwd):$(pwd) -w $(pwd) --cap-add=NET_ADMIN PPGA
```
and using
```sh
TODO
```
to spawn additional terminals as needed.

PPGA has been tested on Arch and Ubuntu for x86-64 architecture, and on MacOS on AppleSilicon.

To compile PPGA (also required in the Docker container), do the following:
```sh
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j8
```


## Running PPGA

PPGA is easiest to run using the *run_client* and *run_server* binaries, explained in the following.

**Warning:** Except for *pid*, *cid*, *inputfile*, and *offset*, all servers and clients must use the
same arguments. In case of mismatches, random faults, crashes, etc. may occur as PPGA currently makes it
solely the user's responsibility to check that servers and clients run on compatible settings.

#### Running Servers Using *run_server*

PPGA requires to run three different servers separately. Each server can be started via *run_server*,
taking the following mandatory arguments (documentation is also shown when running ```./run_server --help```):

- *algorithm* (e.g., ```--algorithm 0```): This is the ID of the graph algorithm to be executed. Options are
    - 0: BFS. This is a simple breadth-first search. Source node(s) are specified by the client inputs.
    - 1: BFS, 3 parallel. This runs three BFS instances in parallel, each starting from one of the first
        three nodes as source. Note that PPGA currently only supports to output the results of the first
        instance to clients.
    - 2: Pi_M. This is the multilayer truncated Katz centrality score, see Section 2.2
        [here](https://eprint.iacr.org/2025/652) for a description. Note that this behaves like the
        standard truncated Katz centrality score for simple graphs, i.e., graphs where there are no
        duplicate edges. Pi_M **requires to also pass weights** as a CLI argument, described later.
    - 3: Pi_K. This is the truncated Katz centrality score, see Section 2.2
        [here](https://eprint.iacr.org/2025/652) for a description. On simple graphs, this is
        equivalent to Pi_M; hence, if it is known that a graph is simple, Pi_M is preferrable as it
        has lower cost. For multigraphs, Pi_K removes any duplicate edges first to ensure that Katz
        centrality still is computed correctly. Pi_K **requires to also pass weights** as a CLI
        argument, described later.
    - 4: Pi_R. Reach centrality score. This simply counts for each nodes how many nodes are reachable
        from that node within the given depth.
    - 5: Custom. This is an interface to run a custom user-defined algorithm without the need of changing
        the *run_server* binary. For more explanation, see TODO.
- *depth* (e.g., ```--depth 3```): This is the iteration depth, i.e., the number of message-passing iterations to be run.
- *pid* (e.g., ```--pid 0```): Server ID. This specifies the unique role of the server. PPGA requires
        to start compute servers with IDs 0 and 1 and an additional dealer server with ID 2.
- *localhost* (```--localhost```) or *net-config* (e.g., ```--net-config ../ip-addresses.json```):
        This specifies how the servers and clients connect to eachother.
    - Using *localhost* without any value lets all servers and clients connect via localhost. This is
        intended for local testing.
    - Using *net-config*, a path to a JSON file is specified containing the IP addresses of all servers
        and clients. The format is as follows: ```["Server 0 IP",  "Server 1 IP", "Server 2 IP", "Client 1 IP", "Client 2 IP", ...]```

Further important arguments:

- *weights* (e.g., ```--weights {3,2,1}```): List of weights with a length that must match **depth**.
            These are the weight parameters to the (multilayer) truncated Katz score Pi_M/Pi_K.
- *bits* (e.g., ```--bits 10```): Number of bits required to represent all node IDs, i.e., each node
            ID needs to be smaller than 2^*bits*. By default, this is set to 32, but any smaller
            possible value can greatly increase performance. It is assumed that clients and servers
            coordinate in advance how many bits are needed for representing nodes.
            *Note that PPGA always uses unsigned 32 bit ints for representing values in the algorithm!*
            This argument simply specifies the practically used bits for IDs, rendering specific
            subcomponents (the underlying radix-sort) of the message-passing more efficient.
- *clients* (e.g., ```--clients 2```): The number of clients. If not specified, this defaults to 1.
- *ssd* (```--ssd```): Lets servers store part of their preprocessing information on disk instead of
                        RAM. **Attention!** While this can greatly decrease RAM utilization, note that
                        each protocol run needs to generate fresh preprocessing information, and this
                        can quickly become tens of GiB each run. Using this option can hence be
                        harmful to SSD lifetime, use with caution!

Other arguments:

- *port* (e.g., ```--port 10000```): Base port for connections between the servers.
- *input-port* (e.g., ```--port 4242```): Server ports for receiving input shares from clients.
- *certificate_path* (e.g., ```--certificate_path certs/cert1.pem```): Path to full certificate chain
                        file for TLS server connections.
- *private_key_path* (e.g., ```--private_key_path certs/key1.pem```): Path to private key for TLS server connections.
- *trusted_cert_path* (e.g., ```--trusted_cert_path certs/cert_ca.pem```): Path to trusted root certificate for TLS connections.
- *BLOCK_SIZE* (e.g., ```--BLOCK_SIZE 100000```): Maximum number of 32bit values to be sent at once
                to remain below TCP buffer sizes. Can be increased for faster communication if appropriately
                larger buffer sizes are set in the OS.

Example: (on three different terminals)
```sh
./run_server --algorithm 0 --depth 3 --localhost --pid 0 --clients 2
./run_server --algorithm 0 --depth 3 --localhost --pid 1 --clients 2
./run_server --algorithm 0 --depth 3 --localhost --pid 2 --clients 2
```

#### Running Clients Using *run_client*

Clients can be started via *run_client*, taking the following mandatory arguments (documentation is
also shown when running ```./run_client --help```):

- *cid* (e.g., ```--cid 0```): Client ID. Clients need to be numbered 0, 1, 2, ...
- *localhost* (```--localhost```) or *net-config* (e.g., ```--net-config ../ip-addresses.json```):
        This specifies how the servers and clients connect to eachother.
    - Using *localhost* without any value lets all servers and clients connect via localhost. This is
        intended for local testing.
    - Using *net-config*, a path to a JSON file is specified containing the IP addresses of all servers
        and clients. The format is as follows: ```["Server 0 IP",  "Server 1 IP", "Server 2 IP", "Client 1 IP", "Client 2 IP", ...]```

Further important arguments:

- *inputfile* (e.g., ```--inputfile subgraph_0.data```): This is the graph input provided by the client.
                For a detailed explanation of the format, please refer to TODO. If not set, this
                defaults to 'subgraph_[cid].data'
- *offset* (e.g., ```--offset 10```): Each client contributes a part to the graph in list form.
            The offset specifies at which position in the merged list the input provided by this client
            should start. 0 by default.
- *bits* (e.g., ```--bits 10```): Number of bits required to represent all node IDs, i.e., each node
            ID needs to be smaller than 2^*bits*. By default, this is set to 32, but any smaller
            possible value can greatly increase performance. It is assumed that clients and servers
            coordinate in advance how many bits are needed for representing nodes.
            *Note that PPGA always uses unsigned 32 bit ints for representing values in the algorithm!*
            This argument simply specifies the practically used bits for IDs, rendering specific
            subcomponents (the underlying radix-sort) of the message-passing more efficient.
- *clients* (e.g., ```--clients 2```): The number of clients. If not specified, this defaults to 1.

Other arguments:

- *port* (e.g., ```--port 10000```): Base port for connections between the servers.
- *input-port* (e.g., ```--port 4242```): Server ports for receiving input shares from clients.
- *certificate_path* (e.g., ```--certificate_path certs/cert1.pem```): Path to full certificate chain
                        file for TLS server connections.
- *private_key_path* (e.g., ```--private_key_path certs/key1.pem```): Path to private key for TLS server connections.
- *trusted_cert_path* (e.g., ```--trusted_cert_path certs/cert_ca.pem```): Path to trusted root certificate for TLS connections.
- *BLOCK_SIZE* (e.g., ```--BLOCK_SIZE 100000```): Maximum number of 32bit values to be sent at once
                to remain below TCP buffer sizes. Can be increased for faster communication if appropriately
                larger buffer sizes are set in the OS.

Example: (on 2 different terminals)
Next, we run Alice, also changing the number of clients:
```sh
./run_client --cid 0 --localhost --clients 2
 ./run_client --cid 1 --localhost --clients 2 --offset 16
```

#### Testing Different Network Settings

To locally or in a LAN simulate different other network settings, bandwidth and RTT can be set using
[tc](https://www.man7.org/linux/man-pages/man8/tc.8.html).


///////////////////////////////////////////////////////////////////////////////////////////

TODO: also sources for algos
TODO: Certificates
TODO input file format, offset!

### Centrality measures

Using this framework, we instantiate three centrality measures (computed on all nodes v):
- Reach Score (pi_R/pi_1): Nodes reachable in at most D hops from v
- Truncated Katz Score (pi_K/pi_2): Paths up to length D, starting at v, weighted depending on their length.
- Multilayer Truncated Katz Score (pi_M/pi_3): Like pi_K, but counting paths multiple times if they use different edges between the same nodes.
:warning: [Our paper](https://eprint.iacr.org/2025/652) uses naming pi_R/pi_K/pi_M whereas [this prior work](https://doi.org/10.1145/3038912.3052602) uses pi_1/pi_2/pi_3. Throughout the code, we also use naming pi_1/pi_2/pi_3 as the switch to pi_R/pi_K/pi_M came after the code.
We also translate the protocols from [the prior work](https://doi.org/10.1145/3038912.3052602) to our setting and provide implementation within this repository.


# Table of Contents

* [TL;DR](#tldr): Quick walkthrough, demonstrating how to run benchmarks on smaller graph instances, plot them, etc.
* [Environment](#environment): Setting up the environment to compile and run MultiCent, either using Docker or standalone
* [Compiling](#compiling): Compiling MultiCent
* [Running the Protocols](#running-the-protocols): Guide to run individual protocols
* [Reproducing our Benchmarks](#reproducing-our-benchmarks): Guide to reproduce benchmarks as done in the paper
* [Parsing and Processing Benchmark Data](#parsing-and-processing-benchmark-data): Guide to parse and display benchmark data as plot and tables
* [Running Tests](#running-tests): Guide on running test cases
* [Repository Content](#repository-content): Outline of which code parts to find where







# Running Tests

To run tests, first [recompile](#Compiling) for debug mode.
Then, navigate to build/benchmarks and run
```sh
./run_test.sh [id]
```
Depending on the value of [id], this will run one of the following tests (0-5 are for building blocks):

0. test: Small test circuit with *, +, XOR, AND gates; test for correct output and communication
1. shuffle: Test shuffle implementation for vectors of size 10, test for correct output and communication
2. doubleshuffle: Test modified 'doubleshuffle' implementation for vectors of size 10, test for correct output and communication
3. compaction: Compute compaction of vector of size 10, test for correct output and communication
4. sort: Sort vector of size 10, test for correct output and communication
5. equalszero: Compute EQZ and then Bit2A on multiple values, test for correct output and communication
6. pi_3_test: pi_3 (D=4) on concrete graph instance with 4 nodes and 6 edges, test for correct output and communication
7. pi_3_benchmark: pi_3 (D=0) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
8. pi_3_benchmark: pi_3 (D=1) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
9. pi_3_benchmark: pi_3 (D=2) on synthetic graph instance with 10 nodes, total size 20, test for correct communication only
10. pi_3_ref_test: reference pi_3 (D=4) on concrete graph instance with 4 nodes and 6 edges, 3 layers, test for correct output and communication
11. pi_3_ref_benchmark: pi_3 (D=1) on synthetic graph instance with 10 nodes, 3 layers, test for correct communication only
12. pi_3_ref_benchmark: pi_3 (D=2) on synthetic graph instance with 10 nodes, 3 layers, test for correct communication only
13. pi_2_test: like above, but for pi_2
14. pi_2_benchmark: like above, but for pi_2
15. pi_2_benchmark: like above, but for pi_2
16. pi_2_benchmark: like above, but for pi_2
17. pi_2_ref_test: like above, but for pi_2
18. pi_2_ref_benchmark: like above, but for pi_2
19. pi_2_ref_benchmark: like above, but for pi_2
20. pi_1_test: like above, but for pi_1
21. pi_1_benchmark: like above, but for pi_1
22. pi_1_benchmark: like above, but for pi_1
23. pi_1_benchmark: like above, but for pi_1
24. pi_1_ref_test: like above, but for pi_1
25. pi_1_ref_benchmark: like above, but for pi_1
26. pi_1_ref_benchmark: like above, but for pi_1


# Repository Content

Note that the overall structure of the repository stems from the structure of [Graphiti](https://github.com/Bhavishrg/Graphiti).
In the following, we briefly outline which essential parts of MultiCent implementation can be found where: 
* [benchmark/subcircuits.h](benchmark/subcircuits.h) and [benchmark/subcircuits.cpp](benchmark/subcircuits.cpp) generate the circuits for all centrality measures. **Here, it can be found how our different high-level protocols such as computation of centrality measures, sorting, etc. are composed of low-level atomic building blocks such as shuffling, multiplications, etc.**
* benchmark contains all benchmarks and test cases. Hence, it also demonstrates how our protocols can be instantiated and used. The benchmarks essentially define the inputs and parameters for the circuit and then let [benchmark/subcircuits.cpp](benchmark/subcircuits.cpp) generate the circuit itself.
* [src/graphsc/offline_evaluator.cpp](src/graphsc/offline_evaluator.cpp) contains the preprocessing for all atomic building blocks. In contrast to Graphiti, building blocks such as shuffling are fixed here and further building blocks like double shuffling etc. are introduced.
* [src/graphsc/online_evaluator_load_balanced.cpp](src/graphsc/online_evaluator_load_balanced.cpp) contains the online phase for all atomic building blocks. In contrast to Graphiti, building blocks such as shuffling are fixed here and further building blocks like double shuffling and more efficient propagation etc. are introduced.

Comparing to Graphiti, all centrality measure benchmarks and [benchmark/subcircuits.h](benchmark/subcircuits.h)/[benchmark/subcircuits.cpp](benchmark/subcircuits.cpp) are completely new, alongside all provided scripts for benchmarking and evaluating.
Beyond that, it is easiest to compare the modifications to priorly existing files [using GitHub diff](https://github.com/Bhavishrg/Graphiti/compare/main...encryptogroup:MultiCent:main).

