#include <algorithm>
#include <cassert>

#include "../src/graphmpc/bit2a.h"
#include "../src/graphmpc/deduplication.h"
#include "../src/graphmpc/equals_zero.h"
#include "../src/graphmpc/mul.h"
#include "../src/setup/cmdline.h"
#include "../src/utils/sharing.h"

void test_shuffle(bpo::variables_map &opts) {
    std::cout << "--- Test Deduplication ---" << std::endl;

    Party id = (Party)opts["pid"].as<size_t>();
    auto conf = setup::setupProtocol(opts);
    auto network = setup::setupNetwork(opts);
    auto rngs = setup::setupRNGs(opts);

    Party recv = P0;
    size_t size = 10;
    std::unordered_map<Party, std::vector<Ring>> preproc;
    std::vector<Ring> online_vals;
    preproc[P0] = {};
    preproc[P1] = {};
    std::vector<Ring> src = {0, 0, 0, 0, 1, 1, 1, 2, 2, 2};
    std::vector<Ring> dst = {0, 0, 1, 2, 0, 1, 1, 0, 0, 1};
    auto src_shared = share::random_share_secret_vec_2P(id, conf.rngs, src);
    auto dst_shared = share::random_share_secret_vec_2P(id, conf.rngs, dst);

    std::vector<Ring> deduplication(9);
    std::vector<Ring> src_dupl(9);
    std::vector<Ring> dst_dupl(9);
    std::vector<Ring> src_and_dst(9);

    DeduplicationSub sub1(&conf, &src_shared, &src_dupl);
    DeduplicationSub sub2(&conf, &dst_shared, &dst_dupl);

    EQZ eqz_src_0(&conf, &preproc, &online_vals, &src_dupl, &src_dupl, recv, size - 1, 0);
    EQZ eqz_src_1(&conf, &preproc, &online_vals, &src_dupl, &src_dupl, recv, size - 1, 1);
    EQZ eqz_src_2(&conf, &preproc, &online_vals, &src_dupl, &src_dupl, recv, size - 1, 2);
    EQZ eqz_src_3(&conf, &preproc, &online_vals, &src_dupl, &src_dupl, recv, size - 1, 3);
    EQZ eqz_src_4(&conf, &preproc, &online_vals, &src_dupl, &src_dupl, recv, size - 1, 4);
    EQZ eqz_dst_0(&conf, &preproc, &online_vals, &dst_dupl, &dst_dupl, recv, size - 1, 0);
    EQZ eqz_dst_1(&conf, &preproc, &online_vals, &dst_dupl, &dst_dupl, recv, size - 1, 1);
    EQZ eqz_dst_2(&conf, &preproc, &online_vals, &dst_dupl, &dst_dupl, recv, size - 1, 2);
    EQZ eqz_dst_3(&conf, &preproc, &online_vals, &dst_dupl, &dst_dupl, recv, size - 1, 3);
    EQZ eqz_dst_4(&conf, &preproc, &online_vals, &dst_dupl, &dst_dupl, recv, size - 1, 4);
    Mul mul(&conf, &preproc, &online_vals, &src_dupl, &dst_dupl, &src_and_dst, recv, true, size - 1);
    Bit2A bit2a(&conf, &preproc, &online_vals, &src_and_dst, &deduplication, recv, size - 1);

    if (id != D) {
        size_t n_recv;
        network->recv(D, &n_recv, sizeof(size_t));
        std::cout << "Receiving " << n_recv << " preprocessing values." << std::endl;
        network->recv_vec(D, n_recv, preproc.at(id));
    }
    eqz_src_0.preprocess();
    eqz_src_1.preprocess();
    eqz_src_2.preprocess();
    eqz_src_3.preprocess();
    eqz_src_4.preprocess();
    eqz_dst_0.preprocess();
    eqz_dst_1.preprocess();
    eqz_dst_2.preprocess();
    eqz_dst_3.preprocess();
    eqz_dst_4.preprocess();
    mul.preprocess();
    bit2a.preprocess();
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

    sub1.evaluate_recv();
    sub2.evaluate_recv();

    auto sub1_rev = share::reveal_vec(id, network, src_dupl);
    auto sub2_rev = share::reveal_vec(id, network, dst_dupl);

    /* EQZ Layer 1 */
    eqz_src_0.evaluate_send();
    eqz_dst_0.evaluate_send();
    size_t n_send = 2 * 18;
    size_t n_recv = 2 * 18;
    std::vector<Ring> data_recv(n_recv);
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz_src_0.evaluate_recv();
    eqz_dst_0.evaluate_recv();

    /* EQZ Layer 1 */
    eqz_src_1.evaluate_send();
    eqz_dst_1.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz_src_1.evaluate_recv();
    eqz_dst_1.evaluate_recv();

    /* EQZ Layer 2 */
    eqz_src_2.evaluate_send();
    eqz_dst_2.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz_src_2.evaluate_recv();
    eqz_dst_2.evaluate_recv();

    /* EQZ Layer 3 */
    eqz_src_3.evaluate_send();
    eqz_dst_3.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz_src_3.evaluate_recv();
    eqz_dst_3.evaluate_recv();

    /* EQZ Layer 4 */
    eqz_src_4.evaluate_send();
    eqz_dst_4.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    eqz_src_4.evaluate_recv();
    eqz_dst_4.evaluate_recv();

    auto src_dupl_rev = share::reveal_vec_bin(id, network, src_dupl);
    auto dst_dupl_rev = share::reveal_vec_bin(id, network, dst_dupl);

    /* AND */
    n_send = 18;
    n_recv = 18;
    mul.evaluate_send();
    if (id == P0) {
        network->send_vec(P1, n_send, online_vals);
        network->recv_vec(P1, n_recv, data_recv);
    } else {
        network->recv_vec(P0, n_recv, data_recv);
        network->send_vec(P0, n_send, online_vals);
    }
    for (size_t i = 0; i < n_recv; ++i) {
        online_vals[i] ^= data_recv[i];
    }
    mul.evaluate_recv();

    auto and_rev = share::reveal_vec_bin(id, network, src_and_dst);

    /* Bit2A */
    bit2a.evaluate_send();
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
    bit2a.evaluate_recv();

    deduplication.insert(deduplication.begin(), 0);

    /* Assertions */
    auto result1 = share::reveal_vec(id, network, deduplication);
    setup::print_vec(result1);

    std::vector<Ring> expected_result({0, 1, 0, 0, 0, 0, 1, 0, 1, 0});
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