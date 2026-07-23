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

# Table of Contents

* [Getting Started: A Quick Demo on Using PPGA](#getting-started-a-quick-demo-on-using-ppga): Quick
    introduction to compiling PPGA in a Docker container and running your first PPGA graph computation.
* [Installing PPGA](#installing-ppga): Detailed information on setting up PPGA standalone or in a Docker container,
    and on compiling the code.
* [Running PPGA](#running-ppga): Instructions on running different graph algorithms on inputs provided
    by multiple clients.
* [Input format](#input-format): Guide to the PPGA graph input format.
* [Algorithms](#algorithms): Overview of included algorithms and guide for implementing new custom graph algorithms.
* [Tests & Benchmarks](#tests--benchmarks): Documentation for contained test and benchmark targets.
* [Structure of the Codebase](#structure-of-the-codebase): Documentation for the codebase structure
    and its central components.
* [TLS Channels and Certificates](#tls-channels-and-certificates): Note on use of included and custom TLS
    certificates.
* [Extenting PPGA](#extending-ppga): Pointers for modifications and extensions which go beyond what is
    currently provided by our graph algorithm design interface.


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
        the *run_server* binary. For more explanation, see [here](#defining-custom-algorithms).
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
                For a detailed explanation of the format, please refer to [here](#input-format).
                If not set, this defaults to 'subgraph_[cid].data'
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


## Input format

PPGA supports graphs input files in the following format:
```
n 1 1
n 2 42
n 3 0
n 4
e 1 2
e 2 3
e 4 2
...
```
A node is represented by the prefix 'n', followed by the node ID after a space. Additionally, it can
be followed by an initial state value, but this is discared by some algorithms like Katz centrality
and only used for algorithms like BFS. The state value is optional. It can be omitted, in which case
it defaults to 0.

An edge is represented by the prefix 'e', followed by the IDs of first the source and then the target
node. PPGA considers directed graphs. For an undirected edge, two directed edges in both directions
can be used. PPGA also supports multigraphs, so edges do not need to be unique (but node IDs do).

The entire joint graph practically consists of a (secret-shared) list of such entries. When multiple
clients contribute parts of the graph, PPGA considers the case where the joint list is segmented into
multiple slices where each client contributes one. As an example, for a graph with 30 list entries and
3 clients, client 0 could contribute the first 15 entries, client 1 the next 10 entries, and client 2
the final 5 entries. In this case, the clients need to specify offsets (see also [here](#running-clients-using-run_client)) as follows:
- Client 0 uses offset 0, as it contributes the slice [0:15].
- Client 1 uses offset 15, as it contributes the slice [15:25].
- Client 2 uses offset 25, as it contributes the slice [25:30].
The end of each slice is automatically derived from the subgraph input file.
**Note that it must be ensures that there remain no gaps between the slices.** Otherwise, the servers
abort the computation.


## Algorithms

PPGA provides multiple graph algorithms out of the box, and provides an intuitive interface to implement
more custom algorithms.

### Included Algorithms

PPGA includes the following algorithms:

- [BFS](algorithms/bfs.h)
- [BFS 3 parallel](algorithms/bfs_3_parallel.h)
- [Pi_M/Multilayer truncated Katz centrality](algorithms/pi_m.h)
- [Pi_K/truncated Katz centrality](algorithms/pi_k.h)
- [Pi_R/Reach centrality](algorithms/pi_r.h)

BFS is a simple breadth-first search, where the source node(s) are taken from the client inputs.
PPGA also includes a variant that runs 3 instances in parallel, one starting at the first node, one
starting at the second node, and one starting at the third.

PPGA also supports multiple centrality scores, translated to message passing as proposed
[here](https://eprint.iacr.org/2025/652). Truncated [Katz centrality](https://en.wikipedia.org/wiki/Katz_centrality)
counts the number of paths starting at each node up to a certain length (our number of iterations).
Paths are weighted by their lengths. The multilayer variant considers paths using different instances
of equal edges in a multigraph separately. Note that for simple graphs, this matches the standard Truncated
Katz score, and if no support for multilayer graphs is required, it is preferred to use the then
equivalent multilayer variant as it's computation costs less. The reach score simply counts the number
of nodes reachable within the distance specified by the number of iterations, from each individual
node as starting point.

### Defining Custom Algorithms

PPGA is intended to make it easy to define custom message-passing algorithms. These can simply be added
to the [algorithms dir](algorithms). There already is a [template file](algorithms/template.h) contained.
We recommend to start by simply modifying [the custom algorithm](algorithms/custom.h), as this can
be run out-of-the-box using the steps from [here](#running-servers-using-run_server), using algorithm
ID **5**, without the need of adding new compile targets.

In short, a custom algorithm consists of the following operations:
- A constructor, which can simply follow the provided template.
- Optionally, a method ```init_nodes(size_t column)``` which sets the initial node states. If this is
    omitted, the user-provided input states will be used instead
- Optionally, a method ```prepare(mp_val &state, size_t i, size_t column)``` which, given a node's
    state, computes the message to be propagated to the node's outgoing edges. If this is omitted,
    the message will automatically be set to the node state.
- (Messages will then be propagated to outgoing edges, and gathered for each node from its incoming
    edges, computing the sum over these messages which is provided as an update value. These steps are
    static parts of PPGA, they cannot be customized.)
- A method ```apply(mp_val &state, mp_val &update, size_t i, size_t column)```which, given a node's state
    and the gathered udpate, determines the new node state.
- Optionally, a method ```post_mp(mp_val &state, size_t column)``` which can run one final operation
    on each node's state after the final message-passing iteration. It this is omitted, the state is
    kept as is.
- Optionally, a method ```post_mp_aggregate(std::vector<mp_val> &states)``` which can aggregate multiple
    columns (explained below) into a single one as the final step after ```post_mp(...)```.

Note that most methods receive additional arguments ```size_t i``` and ```size_t column```.
*i* is the number of the current iteration, enabling to provide different behavior of a method at
different stages of the algorithm. *column* is used to specify the column that is operated on, as
described next.

PPGA supports to run multiple message-passing instances in parallel, using multiple *columns*.
Each column can be understood as its own collection of one node state per node, forming one column
in a list-based representation of the graph. Message-passing runs in parallel on all columns, but
the data from one column cannot be accessed for computation on another column. The methods documented
above receive the column id as input to enable customizing the operations depending on the column,
adding support to run different computations on different columns.
To use multiple columns, the algorithm must specify the number of columns when calling the superclass
constructor, e.g., ```Circuit(conf, 3)``` for three columns. Also, right before the output phase,
```post_mp_aggregate(std::vector<mp_val> &states)``` receives all columns of node states and can
be used to aggregate them into a single state per node.

All algorithms are subclasses of the [Circuit class](src/core/circuit.h) which contains extensive
documentation on the different methods. It also provides a library of pre-defined functions that
can be used to define the message-passing methods. For example, it provides functions
```add(mp_val &x, mp_val &y)``` and ```multiply_by_constant(mp_val &x, Ring constant)```.
Suppose that for each node v with state *s_v* receiving an update *u_v*, the new state should be set
to *s_v + 2 * u_v*. This can then simply be expressed in PPGA as
```C++
mp_val apply(mp_val &state, mp_val &update, size_t /*i*/, size_t /*column*/) override {
    return add(state, multiply_by_constant(update, 2));
}
```
:warning: **Never run arithmetic operators ```+,-,*``` etc on ```mp_val```s!** In the PPGA backend,
```mp_val``` practically represents an address and not a value, so running arithmetics on these addresses
will lead to unintended and perhaps undefined behavior. Instead, always use the custom functions provided
by the [Circuit class](src/core/circuit.h).

The [Circuit class](src/core/circuit.h) also contains some general options:
- *use_reverse_message_passing* can be used to cause message passing to traverse edges backwards, i.e.,
    from destination to source instead of from source to destination. This can be useful, e.g., for
    defining some centrality measures.
- *use_edge_deduplication* converts any input that is a multigraph to a simple graph by removing any
    duplicate edges.
Both options can be used by calling the corresponding methods with the same name in the algorithm's
constructor.


## Tests & Benchmarks

PPGA comes with tests and benchmarks for the included algorithms.
Tests can be found in the [tests](tests) directory and benchmark binaries in the [benchmarks](benchmarks)
directory. Refer to their specific help pages (```./[binary] --help```) for details on the available
options and required arguments.

The tests run on a specific, hardscripted graph, and run assertions to check correctness of the
outputs. Tests and benchmarks both also run assertions to check that the amount of communication matches
the protocol specification. Note that the assertions require to compile the code to debug mode:
```sh
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j8
```


## Structure of the Codebase

PPGA contains of three main components:

#### The Algorithms Component

This includes all graph algorithms and provides space for more custom user defined algorithms.
All algorithms should be placed [here](algorithms).

#### The Data Interface Component

This is located [here](src/data_interface) and contains interfaces for clients to provide inputs to
and receive outputs from servers, and for the protocol configuration etc.

#### The Core Component

Located [here](src/core), this contains the backend of PPGA that takes care of running the graph
algorithms in MPC.
- [io](src/core/io) contains the network layer of PPGA as well as utility to save and retrieve preprocessing
    material from disk.
- [circuit](src/core/circuit.h) defines the circuit to be evaluated, essentially representing a function
    that has a static program flow to not leak any information about inputs. This class contains
    multiple abstract methods to be specified by graph algorithms, as all graph algorithms are subclasses
    of the Circuit class. Furthermore, the Circuit class provides functions for arithmetic operations
    that can be utilized to implement the abstract methods for specific graph algorithms.
- [preprocessor](src/core/preprocessor.h) produces preprocessing material for a circuit to later speed
    up the online execution once that inputs are provided.
- [storage](src/core/storage.h) stores the preprocessing material produced by the preprocessor and provides
    it to the evaluator.
- [evaluator](src/core/evaluator.h) executes a circuit, using the provided inputs and preprocessing material,
    to evaluate a graph algorithm.
- [gate](src/core/gate.h) defines individual gates, atomic operations of a circuit.

#### Further Parts

- [cmdline](src/setup/cmdline.h) acts as a helper to bootstrap execution of servers and clients from CLI arguments.
- [utils](src/utils) contains diverse utilities for PRGs, secret-sharing, processing plaintext graphs,
    secret-sharing graphs, measuring performance, etc.
- [graph](src/utils/graph.h), specifically, defines how graphs are represented as a "DAG list".
- [benchmarks](benchmarks) contains targets specifically for benchmarking algorithm performance. See [here](#tests--benchmarks).
- [tests](tests) contains targets to run tests for algorithms on specific input graphs. See [here](#tests--benchmarks).


## TLS Channels and Certificates

PPGA uses TLS to establish secure channels between pairs of servers and/or clients.
In [the certs dir](certs), there are mock certificates for the servers for testing and demonstation
purposes. Note that in a more realistic scenario, it would of course be necessary to properly set
up certificates so that only the intended servers have access to their respective private key


## Extending PPGA

For modifications which are not covered by the interface provided by [circuit.h](src/core/circuit.h),
it may be necessary to modify the backend core. While different needs may require vastly different
modifications at different places, we document as an example here how a new custom operation could
be added to the interface of [circuit.h](src/core/circuit.h) to be used in custom graph algorithms.
Note that this requires some MPC protocol engineering experience.
- [circuit.h](src/core/circuit.h) (and the implementation file) needs to be extended by a function
    providing the interface to the new operation. Its implementation needs to add one or multiple gates
    to the circuit. For novel custom operations, it is likely that new gates need to be introduced.
    Note that each gate needs to require at most one MPC communication round, so it might be necessary to define the function from a chain of gates or similar.
- [gate.h](src/core/gate.h) (and its implementation file) needs to be extended by newly required gates.
    Here, it is important what the inputs and outputs and their dimensions are, and it needs to be
    specified if evaluation of the gate requires interaction or not.
- [preprocessor](src/core/preprocessor.h) and [evaluator](src/core/evaluator.h) (and their implementation
    files) need to be extended to define how the gate can be evaluated in MPC. The preprocessor, for
    each interactive gate, can be used to let the dealer server compute preprocessing material to provide
    to the other servers. The evaluator, for each gate, specifies computation before the interaction,
    defining data to be send to the other server, and computation after the interaction, using data
    received from the other server and finalizing the gate processing. Non-interactive gates must run
    all their operations in the part executed after the communication round.

In case of questions, or if you need any help, please do not hesitate to open an issue on the PPGA github page.
