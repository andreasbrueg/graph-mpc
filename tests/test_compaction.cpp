#include <algorithm>
#include <cassert>

#include "../src/graphmpc/compaction.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/sharing.h"

void test_shuffle(bpo::variables_map &opts) {
    std::cout << "--- Test Shuffle ---" << std::endl;

    Party id = (Party)opts["pid"].as<size_t>();
    auto conf = setup::setupProtocol(opts);
    auto network = setup::setupNetwork(opts);
    auto rngs = setup::setupRNGs(opts);

    Party recv = P0;
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> online_vals;
    preproc[P0] = {};
    preproc[P1] = {};

    std::vector<Ring> test_vector = {0, 1, 1, 0, 1, 0, 0, 1, 0, 1};
    auto test_vector_shared = share::random_share_secret_vec_2P(id, conf.rngs, test_vector);
    std::vector<Ring> sorted_vector1(10);
    std::vector<Ring> sorted_vector2(10);

    Compaction compaction1(&conf, &preproc, &online_vals, &test_vector_shared, &sorted_vector1, recv);
    Compaction compaction2(&conf, &preproc, &online_vals, &test_vector_shared, &sorted_vector2, recv);

    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        std::cout << "Receiving " << n_recv << " preprocessing values." << std::endl;
        network->recv_vec(D, n_recv, preproc.at(id));
    }
    compaction1.preprocess();
    compaction2.preprocess();
    if (id == D) {
        for (auto &party : {P0, P1}) {
            auto &data_send = preproc.at(party);
            size_t n_send = data_send.size();
            std::cout << "Sending " << n_send << " preprocessing values." << std::endl;
            network->send(party, &n_send, sizeof(size_t));
            network->send_vec(party, n_send, data_send);
        }
        return;
    }

    compaction1.evaluate_send();
    size_t n_send = 20;
    size_t n_recv = 20;
    std::vector<Ring> data_recv(n_recv);
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] += data_recv[i];
    }
    compaction1.evaluate_recv();

    compaction2.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] += data_recv[i];
    }
    compaction2.evaluate_recv();

    auto result1 = share::reveal_vec(id, network, sorted_vector1);
    auto result2 = share::reveal_vec(id, network, sorted_vector2);
    setup::print_vec(result1);
    setup::print_vec(result2);
    std::vector<Ring> expected_result({0, 5, 6, 1, 7, 2, 3, 8, 4, 9});
    assert(result1 == expected_result);
}

int main(int argc, char **argv) {
    auto prog_opts(setup::programOptionsTest());

    bpo::options_description cmdline("Test for the secure computation of the Reach Score.");
    cmdline.add(prog_opts);
    cmdline.add_options()("config,c", bpo::value<std::string>(), "configuration file for easy specification of cmd line arguments")("help,h",
                                                                                                                                    "produce help message");

    bpo::variables_map opts = setup::parseOptions(cmdline, prog_opts, argc, argv);

    try {
        test_shuffle(opts);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\nFatal error" << std::endl;
        return 1;
    }

    return 0;
}