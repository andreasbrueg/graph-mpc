#include "../setup/utils.h"
#include "../src/io/input_client.h"
#include "../src/io/input_server.h"
#include "../src/utils/graph.h"

#define PRINT_LOG [](const std::string &strLogMsg) { std::cout << strLogMsg << std::endl; }

void test_socket(Party id, RandomGenerators &rngs, io::NetworkConfig &net_conf, size_t n, std::string input_file) {
    std::unique_ptr<CTCPSSLClient> m_pSSLTCPClient;

    if (id == P0) {                                    // Server
        InputServer server(id, std::to_string(4242));  // Socket expecting one client
        server.connect_clients();
        auto pkt = server.recv_packet(0);
        auto entries = pkt.entries;

        Graph g;
        for (size_t i = 0; i < entries.size() / 4; ++i) {
            g.add_list_entry(entries[i * 4], entries[i * 4 + 1], entries[i * 4 + 2], entries[i * 4 + 3]);
        }
        g.print();
    }

    if (id == P1) {  // Client
        Graph g;
        g.add_list_entry(0, 0, 1, 10);
        g.add_list_entry(1, 1, 1, 20);
        g.add_list_entry(2, 2, 1, 30);
        g.add_list_entry(0, 1, 0, 0);
        g.add_list_entry(0, 2, 0, 0);
        g.add_list_entry(1, 2, 0, 0);
        g.add_list_entry(2, 0, 0, 0);
        g.print();

        InputClient::Packet pkt;
        pkt.start = 0;
        pkt.end = g.size() * 4;
        pkt.entries = g.serialize();

        InputClient client(id);
        client.connect("localhost", 4242);
        client.send_graph(g);
    }

    return;
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptions());

    bpo::options_description cmdline("Run a simple test for sorting.");
    cmdline.add(prog_opts);

    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        setup::run_test(opts, test_socket);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}