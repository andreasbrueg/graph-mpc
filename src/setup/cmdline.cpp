#include "cmdline.h"

void setup::print_vec(std::vector<Ring> &vec) {
    for (size_t i = 0; i < vec.size(); ++i) {
        if (i != vec.size() - 1) {
            std::cout << vec[i] << ", ";
        } else {
            std::cout << vec[i] << std::endl;
        }
    }
}

bpo::options_description setup::programOptionsBenchmark() { // TODO remove, kinda duplicate
    bpo::options_description desc(
        "Following options are supported by config file too. Regarding seeds, "
        "all, 01, 02, and 12 need to be equal among the parties using them. "
        "Other parties, e.g., party 2 for seed 01 can take any value and in "
        "fact must not know the value that 0 and 1 use");

    desc.add_options()(
        "pid,p", bpo::value<int>()->default_value(0), "Party ID.")(
        "isclient", bpo::bool_switch(), "Defines if user is a client.")(
        "parties", bpo::value<size_t>()->default_value(3), "Number of parties running the protocol.")(
        "seed_self_h", bpo::value<uint64_t>()->default_value(100), "Value of the private random seed, high bits.")(
        "seed_self_l", bpo::value<uint64_t>()->default_value(0), "Value of the private random seed, low bits (will add pid if 0/default).")(
        "seed_D0_recv_h", bpo::value<uint64_t>()->default_value(4), "Value of the D0-shared random seed for evaluate_recv, high bits.")(
        "seed_D0_recv_l", bpo::value<uint64_t>()->default_value(8), "Value of the D0-shared random seed for evaluate_recv, low bits.")(
        "seed_D1_recv_h", bpo::value<uint64_t>()->default_value(15), "Value of the D1-shared random seed for evaluate_recv, high bits.")(
        "seed_D1_recv_l", bpo::value<uint64_t>()->default_value(16), "Value of the D1-shared random seed for evaluate_recv, low bits.")(
        "seed_01_h", bpo::value<uint64_t>()->default_value(23), "Value of the 01-shared random seed, high bits.")(
        "seed_01_l", bpo::value<uint64_t>()->default_value(42), "Value of the 01-shared random seed, low bits.")(
        "seed_D0_send_h", bpo::value<uint64_t>()->default_value(108), "Value of the D0-shared random seed for evaluate_send, high bits.")(
        "seed_D0_send_l", bpo::value<uint64_t>()->default_value(1337), "Value of the D0-shared random seed for evaluate_send, low bits.")(
        "seed_D1_send_h", bpo::value<uint64_t>()->default_value(21108), "Value of the D1-shared random seed for evaluate_send, high bits.")(
        "seed_D1_send_l", bpo::value<uint64_t>()->default_value(211337), "Value of the D1-shared random seed for evaluate_send, low bits.")(
        "seed_D0_prep_h", bpo::value<uint64_t>()->default_value(1108), "Value of the D0-shared random seed for preprocessing, high bits.")(
        "seed_D0_prep_l", bpo::value<uint64_t>()->default_value(11337), "Value of the D0-shared random seed for preprocessing, low bits.")(
        "seed_D1_prep_h", bpo::value<uint64_t>()->default_value(1111108), "Value of the D1-shared random seed for preprocessing, high bits.")(
        "seed_D1_prep_l", bpo::value<uint64_t>()->default_value(11111337), "Value of the D1-shared random seed for preprocessing, low bits.")(
        "net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all parties.")(
        "localhost", bpo::bool_switch(), "All parties are on same machine.")(
        "size,s", bpo::value<size_t>()->default_value(10), "Number of graph entries.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "nodes", bpo::value<size_t>()->default_value(5), "Number of nodes for benchmarking graph algorithms")(
        "output,o", bpo::value<std::string>(), "File to save benchmarks.")(
        "repeat,r", bpo::value<size_t>()->default_value(1), "Number of times to run benchmarks.")(
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(100000), "BLOCK_SIZE for sending messages over the network.")(
        "ssd", bpo::bool_switch(), "Preprocessing values are stored on disk before they are sent.")(
        "input,i", bpo::value<std::string>(), "File specifying the graph.")(
        "depth,d", bpo::value<size_t>()->default_value(0), "search depth parameter D")(
        "clients", bpo::value<size_t>()->default_value(0), "Number of clients participating in input-sharing.")(
        "input-port", bpo::value<int>()->default_value(4242), "Port used for receiving input shares from clients.")(
        "save-seeds", bpo::bool_switch(), "Toggle saving of PRG-seeds to a file. Needed if parties go offline after preprocessing.");

    return desc;
}

bpo::options_description setup::programOptionsTest() {
    bpo::options_description desc(
        "Following options are supported by config file too.");

    desc.add_options()(
        "pid,p", bpo::value<int>()->default_value(0), "Party ID.")(
        "isclient", bpo::bool_switch(), "Defines if user is a client.")( // TODO remove, should be other executable
        "net-config", bpo::value<std::string>(), "Path to JSON file containing network details of all parties.")(
        "localhost", bpo::bool_switch(), "All parties are on same machine.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert1.pem"), "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")(
        "parties", bpo::value<size_t>()->default_value(3), "Number of parties running the protocol.")( // TODO is there any other option? Remove?
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(100000), "BLOCK_SIZE for sending messages over the network.")( // TODO add better doc
        "ssd", bpo::bool_switch(), "Preprocessing values are stored on disk before they are sent.")( // TODO better doc
        "input,i", bpo::value<std::string>(), "File specifying the graph.")( // TODO investigate
        "clients", bpo::value<size_t>()->default_value(0), "Number of clients participating in input-sharing.")( // TODO zero leads to errors!!
        "size,s", bpo::value<size_t>()->default_value(10), "Number of graph entries.")( // TODO why default?
        "nodes", bpo::value<size_t>()->default_value(0), "Number of nodes for benchmarking graph algorithms")( // TODO why default?
        "depth,d", bpo::value<size_t>()->default_value(0), "search depth parameter D"); // TODO why default?

    return desc;
}

bpo::options_description setup::clientProgramOptions() {
    bpo::options_description desc("Following options are supported by config file too.");

    desc.add_options()("isclient", bpo::bool_switch(), "Defines if user is a client.")("cid", bpo::value<int>()->default_value(0), "Client ID.")(
        "start", bpo::value<size_t>()->default_value(0), "Start index of subgraph")(
        "input,i", bpo::value<std::string>(), "Input file from which subgraph is read.")("bits", bpo::value<size_t>()->default_value(32), "Bits used.")(
        "certificate_path", bpo::value<std::string>()->default_value("certs/cert_client_1.pem"),
        "Path to full certificate chain file for TLS server connections")(
        "private_key_path", bpo::value<std::string>()->default_value("certs/key_client_1.pem"), "Path to private key for TLS server connections")(
        "trusted_cert_path", bpo::value<std::string>()->default_value("certs/cert_ca.pem"), "Path with trusted certificate for TLS client connections")(
        "port", bpo::value<int>()->default_value(10000), "Base port for networking.")("clients", bpo::value<size_t>()->default_value(1),
                                                                                      "Number of clients participating in input-sharing.")(
        "BLOCK_SIZE", bpo::value<size_t>()->default_value(100000), "BLOCK_SIZE for sending messages over the network.")(
        "localhost", bpo::bool_switch(), "All parties are on same machine.")("net-config", bpo::value<std::string>(),
                                                                             "Path to JSON file containing network details of all parties.");
    return desc;
}

bpo::variables_map setup::parseOptions(bpo::options_description &cmdline, bpo::options_description &prog_opts, int argc, char *argv[]) {
    bpo::variables_map opts;
    bpo::store(bpo::command_line_parser(argc, argv).options(cmdline).run(), opts);

    if (opts.count("help") != 0) {
        std::cout << cmdline << std::endl;
        std::exit(0);
    }

    if (opts.count("config") > 0) {
        std::string cpath(opts["config"].as<std::string>());
        std::ifstream fin(cpath.c_str());

        if (fin.fail()) {
            std::cerr << "Could not open configuration file at " << cpath << "\n";
            throw std::runtime_error("Conf file missing");
        }

        bpo::store(bpo::parse_config_file(fin, prog_opts), opts);
    }

    try {
        bpo::notify(opts);

        if (!opts["localhost"].as<bool>() && (opts.count("net-config") == 0)) {
            throw std::runtime_error("Expected one of 'localhost' or 'net-config'");
        }

    } catch (const std::exception &ex) {
        std::cerr << ex.what() << std::endl;
        throw std::runtime_error("Error occured");
    }

    return opts;
}

std::shared_ptr<io::NetIOMP> setup::setupNetwork(const bpo::variables_map &opts) {
    bool is_client = opts["isclient"].as<bool>();
    size_t parties;
    int id;

    if (is_client) {
        id = opts["cid"].as<int>();
        id += 3;  // Client id's start with 3
        parties = 3;
    } else {
        id = opts["pid"].as<int>();
        parties = opts["parties"].as<size_t>();
    }

    size_t clients = opts["clients"].as<size_t>();
    int port = opts["port"].as<int>();
    auto certificate_path = opts["certificate_path"].as<std::string>();
    auto private_key_path = opts["private_key_path"].as<std::string>();
    auto trusted_cert_path = opts["trusted_cert_path"].as<std::string>();
    size_t BLOCK_SIZE = opts["BLOCK_SIZE"].as<size_t>();

    std::vector<std::string> IP;
    bool localhost;
    if (opts["localhost"].as<bool>()) {
        localhost = true;
    } else {
        std::ifstream fnet(opts["net-config"].as<std::string>());
        if (!fnet.good()) {
            fnet.close();
            throw std::runtime_error("Could not open network config file");
        }
        json netdata;
        fnet >> netdata;
        fnet.close();

        IP.resize(3);
        for (size_t i = 0; i < 3; ++i) {
            IP[i] = netdata[i].get<std::string>();
        }
        localhost = false;
    }

    NetworkConfig net_conf = {id, parties, clients, BLOCK_SIZE, port, IP, certificate_path, private_key_path, trusted_cert_path, localhost};
    bool force_connect = clients > 0;
    return std::make_shared<io::NetIOMP>(net_conf, force_connect);
}

void setup::setupClient(const bpo::variables_map &opts, size_t &start_idx, size_t &bits, std::string &input_file) {
    start_idx = opts["start"].as<size_t>();
    input_file = opts["input"].as<std::string>();
    bits = opts["bits"].as<size_t>();
}

void setup::setupServer(const bpo::variables_map &opts, Graph &g, std::shared_ptr<io::NetIOMP> network) {
    int id = opts["pid"].as<int>();
    size_t clients = opts["clients"].as<size_t>();

    if (clients > 0) {
        if (id != D) {
            InputServer server(network, clients);
            std::cout << "Awaiting " << clients << " packets" << std::endl << std::endl;
            g = server.recv_graph();
            // g.init_mp((Party)id); // TODO remove
            std::cout << "Finished graph construction." << std::endl;
        } else {
            size_t nodes, size;
            InputServer server(network, clients);
            nodes = server.recv_nodes();
            server.recv_size(size);
            g.nodes = nodes;
            g.size = size;
        }
    }
}

ProtocolConfig setup::setupProtocol(const bpo::variables_map &opts, std::shared_ptr<io::NetIOMP> network) {
    int pid;
    size_t size, nodes, depth, bits;
    bool ssd;

    pid = opts["pid"].as<int>();
    size = opts["size"].as<size_t>();
    nodes = opts["nodes"].as<size_t>();
    depth = opts["depth"].as<size_t>();

    ssd = opts["ssd"].as<bool>();
    bits = std::ceil(std::log2(nodes + 2));

    return {(Party)pid, size, nodes, depth, bits, ssd};
}

BenchmarkConfig setup::setupBenchmark(const bpo::variables_map &opts) {
    std::string input_file, save_file;
    size_t repeat;
    bool save_output;

    save_output = false;
    if (opts.count("output") != 0) {
        save_output = true;
        save_file = opts["output"].as<std::string>();
    }

    if (opts.count("input") != 0) {
        input_file = opts["input"].as<std::string>();
    }

    repeat = opts["repeat"].as<size_t>();

    return {input_file, save_file, repeat, save_output};
}
